/*
 * Copyright (c) 2020-present, Weibo, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author ZhongXiu Hao <nmred.hao@gmail.com>
 * @author liubang <it.liubang@gmail.com>
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#include "router.h"

#include "city.h"
#include "common/util.h"
#include "service_info_puller.h"

namespace service_router {

// use in thrift.h
DEFINE_int32(discover_wait_milliseconds, 100, "Discover client max wait milliseconds");
DEFINE_int32(idle_timeout, 60000, "Thrift server connection idle timeout");
DEFINE_int32(thrift_connection_retry, 1, "Thrift client connection server retry times");
DEFINE_int32(thrift_timeout_retry, 0, "Thrift client call server process timeout retry times");
DEFINE_int32(router_callback_threads, 2, "Router callback cpu threads");
DEFINE_string(global_registry_type, "consul", "Type for global registry {consul|agent}");
DEFINE_string(global_registry_addresses, "", "Address for global registry");

DECLARE_string(router_consul_addresses);

constexpr char ROUTER_METRICS_MODULE_NAME[] = "service_router";
constexpr char ROUTER_METRICS_SELECT_ADDRESS[] = "select_address";
constexpr char ROUTER_METRICS_DISCOVER_TIMERS[] = "discover";
constexpr char ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR[] = "addr";
constexpr char ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR_VAL_NONE[] = "none";
constexpr uint32_t ROUTER_DEFAULT_SHARD_ID = 4294967295;  // 对于没有分片的服务默认传递 UINT32_MAX

ServerList::ServerList(const std::vector<Server>& server_list) : servers_(server_list) {
  for (auto& server : server_list) {
    std::string hash_str = folly::to<std::string>(server.getHost(), server.getPort());
    hash_code_ = CityHash64WithSeed(hash_str.data(), hash_str.size(), hash_code_);
  }
}

folly::Optional<Server> ServerList::selectServers(const std::shared_ptr<LoadBalanceInterface>& load) const {
  Server server;
  if (load->select(&server, servers_)) {
    return server;
  }

  return folly::none;
}

const std::vector<Server> ServerList::getServers() { return servers_; }

ServiceRegistry::ServiceRegistry(const std::unordered_map<int64_t, std::shared_ptr<ServerList>>& services)
    : services_(services) {}

folly::Optional<std::shared_ptr<ServerList>> ServiceRegistry::selectServers(int64_t key) {
  auto iter = services_.find(key);
  if (iter != services_.end()) {
    return iter->second;
  }
  return folly::none;
}

const std::unordered_map<int64_t, std::shared_ptr<ServerList>> ServiceRegistry::getServices() { return services_; }

RouterDb::RouterDb() {
  auto metrics = metrics::Metrics::getInstance();
  timers_ = metrics->buildTimers(ROUTER_METRICS_MODULE_NAME, ROUTER_METRICS_DISCOVER_TIMERS, 1, 0, 1000);
  serviceRegistry_ = std::make_shared<ServiceRegistry>();
}

folly::Optional<std::shared_ptr<ServerList>> RouterDb::selectServers(const std::string& service_name,
                                                                     const ServerProtocol& protocol, int64_t route_id,
                                                                     const ShardType& type, const std::string& dc) {
  metrics::Timer timer(timers_.get());
  int64_t key = getServiceKey(service_name, protocol, route_id, type, dc);
  update_server_mutex_.lock_shared();
  auto serverList = serviceRegistry_->selectServers(key);
  update_server_mutex_.unlock_shared();
  return serverList;
}

std::unordered_map<int64_t, std::shared_ptr<ServerList>> RouterDb::selectServers(const std::string& service_name,
                                                                                 const ServerProtocol& protocol,
                                                                                 const std::vector<int64_t>& route_ids,
                                                                                 const ShardType& type,
                                                                                 const std::string& dc) {
  metrics::Timer timer(timers_.get());
  std::unordered_map<int64_t, std::shared_ptr<ServerList>> server_lists;
  for (auto& route_id : route_ids) {
    int64_t key = getServiceKey(service_name, protocol, route_id, type);
    update_server_mutex_.lock_shared();
    auto serverList = serviceRegistry_->selectServers(key);
    update_server_mutex_.unlock_shared();
    if (serverList.hasValue()) {
      server_lists[route_id] = serverList.value();
    }
  }
  return server_lists;
}

const std::vector<Server> RouterDb::pickServers(const std::string& service_name, const std::vector<Server>& list) {
  if (list.empty()) {
    return list;
  }

  std::vector<Server> result;
  std::time_t now = common::currentTimeInMs();
  ServiceRouterConfig config = getRouterConfig(service_name);
  for (auto& t : list) {
    if (static_cast<uint64_t>(now) > t.getUpdateTime() + config.getTtlInMs()) {  // 服务心跳超时
      VLOG(4) << "Server heartbeat is timeout, " << t;
      continue;
    }

    if (t.getStatus() != ServerStatus::AVAILABLE) {
      VLOG(4) << "Server status is: " << t.getStatus();
      continue;
    }
    result.push_back(t);
  }

  if (result.empty()) {
    VLOG(4) << "Service:" << service_name << " list has shutdown, return all list";
    result = list;
  }

  return result;
}

void RouterDb::updateServers(const std::string& service_name, const std::vector<Server>& list) {
  if (list.empty()) {
    VLOG(4) << "Service:" << service_name << " list is empty, so not update service cache.";
  }

  std::vector<Server> pick_result = pickServers(service_name, list);
  std::unordered_map<int64_t, std::vector<Server>> shard_servers;
  std::unordered_map<int64_t, std::vector<Server>> partition_servers;
  for (auto& t : pick_result) {
    t.setHostLong(common::ipToInt(t.getHost()));
    if (t.getIsEdgeNode()) {
      auto partition_list = t.getPartitionList();
      for (auto partition_hash : partition_list) {
        int64_t key = getServiceKey(service_name, t.getProtocol(), partition_hash, ShardType::FOLLOWER, t.getDc());
        if (partition_servers.find(key) != partition_servers.end()) {
          partition_servers[key].push_back(t);
        } else {
          partition_servers[key] = {t};
        }
      }
      continue;
    }

    DCHECK(!t.getIsEdgeNode());
    std::vector<uint32_t> available_shard_list = t.getAvailableShardList();
    std::vector<uint32_t> follower_available_shard_list = t.getFollowerAvailableShardList();
    if (t.getShardList().empty() && t.getFollowerShardList().empty()) {  // 按照不分区处理
      available_shard_list.push_back(ROUTER_DEFAULT_SHARD_ID);
    }
    for (auto shard_id : available_shard_list) {
      int64_t key = getServiceKey(service_name, t.getProtocol(), shard_id, ShardType::LEADER, t.getDc());
      if (shard_servers.find(key) != shard_servers.end()) {
        shard_servers[key].push_back(t);
      } else {
        shard_servers[key] = {t};
      }

      // ShardType::ALL 代表 leader follower 混合返回, 对于读操作很适合，更加有效的降低热点请求
      key = getServiceKey(service_name, t.getProtocol(), shard_id, ShardType::ALL, t.getDc());
      if (shard_servers.find(key) != shard_servers.end()) {
        shard_servers[key].push_back(t);
      } else {
        shard_servers[key] = {t};
      }
    }
    for (auto shard_id : follower_available_shard_list) {
      int64_t key = getServiceKey(service_name, t.getProtocol(), shard_id, ShardType::FOLLOWER, t.getDc());
      if (shard_servers.find(key) != shard_servers.end()) {
        shard_servers[key].push_back(t);
      } else {
        shard_servers[key] = {t};
      }
      key = getServiceKey(service_name, t.getProtocol(), shard_id, ShardType::ALL, t.getDc());
      if (shard_servers.find(key) != shard_servers.end()) {
        shard_servers[key].push_back(t);
      } else {
        shard_servers[key] = {t};
      }
    }
  }

  update_server_mutex_.lock_shared();
  auto currentServices = serviceRegistry_->getServices();
  update_server_mutex_.unlock_shared();
  // clean partition ServerList
  last_partition_keys_.withWLock([&currentServices, &service_name](auto& partition_keys) {
    if (partition_keys.find(service_name) == partition_keys.end()) {
      return;
    }

    auto& keys = partition_keys[service_name];
    for (auto key : keys) {
      currentServices.erase(key);
    }
    keys.clear();
  });

  for (auto& t : shard_servers) {
    currentServices[t.first] = std::make_shared<ServerList>(t.second);
  }

  last_partition_keys_.withWLock([&partition_servers, &currentServices, &service_name](auto& partition_keys) {
    if (partition_keys.find(service_name) == partition_keys.end()) {
      partition_keys[service_name] = std::unordered_set<int64_t>();
    }
    auto& keys = partition_keys[service_name];
    for (auto& t : partition_servers) {
      currentServices[t.first] = std::make_shared<ServerList>(t.second);
      keys.insert(t.first);
    }
  });

  update_server_mutex_.lock();
  serviceRegistry_ = std::make_shared<ServiceRegistry>(currentServices);
  update_server_mutex_.unlock();
}

const std::string RouterDb::getConfig(const std::string& name, const std::string& key) {
  std::string result;
  auto other_configs = getConfig(name).getConfigs();
  if (other_configs.find(key) != other_configs.end()) {
    result = other_configs[key];
  }

  return result;
}

const ServiceRouterConfig RouterDb::getRouterConfig(const std::string& service_name) {
  auto config = getConfig(service_name);
  return config.getRouter();
}

const ServiceConfig RouterDb::getConfig(const std::string& service_name) {
  ServiceConfig config;
  configs_.withRLock([&](auto& configs) {
    if (configs.find(service_name) != configs.end()) {
      config = configs.at(service_name);
    }
  });

  return config;
}

void RouterDb::updateConfig(const std::string& name, const ServiceConfig& config) {
  configs_.withWLock([&](auto& configs) { configs[name] = config; });
}

int64_t RouterDb::getServiceKey(const std::string& service_name, const ServerProtocol& protocol, int64_t route_id,
                                const ShardType& type, const std::string& dc) {
  int64_t key = CityHash64WithSeed(dc.data(), dc.size(), route_id);
  key = CityHash64WithSeed(service_name.data(), service_name.size(), key);
  std::string protocol_str = toStringServerProtocol(protocol);
  key = CityHash64WithSeed(protocol_str.data(), protocol_str.size(), key);
  std::string type_str = folly::to<std::string>(static_cast<uint32_t>(type));
  key = CityHash64WithSeed(type_str.data(), type_str.size(), key);
  return key;
}

folly::Singleton<Router> global_service_router = folly::Singleton<Router>().shouldEagerInit();

std::shared_ptr<Router> Router::getInstance() { return global_service_router.try_get(); }

Router::Router() {
  router_thread_ = std::make_unique<RouterThread>();
  db_ = std::make_unique<RouterDb>();
  evb_ = router_thread_->getEventBase();
  callback_thread_pool_ = std::make_shared<folly::CPUThreadPoolExecutor>(
      FLAGS_router_callback_threads, std::make_shared<folly::NamedThreadFactory>("RouterCallback"));
}

void Router::createServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address) {
  createRegistry(service_name, type, address);
}

void Router::createTargetServiceRegistry(const std::string& service_name, RegistryType type,
                                         const std::string& address) {
  createRegistry(service_name, type, address);
}
void Router::waitForConfig(const std::string& service_name, bool is_once) {
  getOrCreateRegistry(service_name);
  getOrCreateConfigPuller(service_name)->wait(is_once);
}

void Router::waitUntilConfig(const std::string& service_name, int milliseconds, bool is_once) {
  getOrCreateRegistry(service_name);
  getOrCreateConfigPuller(service_name)->waitUntil(milliseconds, is_once);
}

void Router::waitForDiscover(const std::string& service_name, bool is_once) {
  waitForConfig(service_name, is_once);
  getOrCreateDiscoverPuller(service_name)->wait(is_once);
}

void Router::waitUntilDiscover(const std::string& service_name, int milliseconds, bool is_once) {
  waitUntilConfig(service_name, milliseconds, is_once);
  getOrCreateDiscoverPuller(service_name)->waitUntil(milliseconds, is_once);
}

bool Router::registerServerSync(const Server server) {
  std::string key = getServerKey(server);
  bool ret = false;
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) == servers.end()) {
      servers[key] = std::make_shared<ServerWithHeartbeat>(server, evb_, this, db_.get());
    }
    ret = getOrCreateRegistry(server.getServiceName())->registerServerSync(server);
  });
  VLOG(5) << "Register complete.";
  return ret;
}

void Router::registerServer(const Server server) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) == servers.end()) {
      servers[key] = std::make_shared<ServerWithHeartbeat>(server, evb_, this, db_.get());
    }
    getOrCreateRegistry(server.getServiceName())->registerServer(server);
  });
  VLOG(5) << "Register complete.";
}

void Router::setStatus(const Server& server, const ServerStatus& status) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setStatus(status);
    }
  });
}

folly::Optional<ServerStatus> Router::getStatus(const Server& server) {
  std::string key = getServerKey(server);
  ServerStatus status;
  bool ret = servers_.withRLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      status = servers.at(key)->getStatus();
      return true;
    }
    return false;
  });

  if (ret) {
    return status;
  }
  return folly::none;
}

void Router::setWeight(const Server& server, int weight) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setWeight(weight);
    }
  });
}

void Router::setIdc(const Server& server, const std::string& idc) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setIdc(idc);
    }
  });
}

void Router::setDc(const Server& server, const std::string& dc) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setDc(dc);
    }
  });
}

void Router::setShardList(const Server& server, const std::vector<uint32_t>& shard_list) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setShardList(shard_list);
    }
  });
}

void Router::setAvailableShardList(const Server& server, const std::vector<uint32_t>& shard_list) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setAvailableShardList(shard_list);
    }
  });
}

void Router::setFollowerShardList(const Server& server, const std::vector<uint32_t>& shard_list) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setFollowerShardList(shard_list);
    }
  });
}

void Router::setFollowerAvailableShardList(const Server& server, const std::vector<uint32_t>& shard_list) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setFollowerAvailableShardList(shard_list);
    }
  });
}

void Router::setOtherSettings(const Server& server,
                              const std::unordered_map<std::string, std::string>& other_settings) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setOtherSettings(other_settings);
    }
  });
}

void Router::setOtherSettings(const Server& server, const std::string& set_key, const std::string& value) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setOtherSettings(set_key, value);
    }
  });
}

void Router::setStatus(const std::string& service_name, const ServerStatus& status) {
  servers_.withWLock([&service_name, &status](auto& servers) {
    for (auto& serverInfo : servers) {
      auto& server = serverInfo.second->getServer();
      if (server.getServiceName() == service_name) {
        LOG(INFO) << "Service name:" << service_name << " server:" << server << " change status:" << status;
        serverInfo.second->setStatus(status);
      }
    }
  });
}

folly::Optional<ServerStatus> Router::getStatus(const std::string& service_name) {
  ServerStatus status;
  bool ret = servers_.withRLock([&](auto& servers) {
    for (auto& serverInfo : servers) {
      auto& server = serverInfo.second->getServer();
      if (server.getServiceName() == service_name) {
        status = server.getStatus();
        return true;
      }
    }
    return false;
  });

  if (ret) {
    return status;
  }
  return folly::none;
}

void Router::setIsEdgeNode(const Server& server, bool is_edge_node) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setIsEdgeNode(is_edge_node);
    }
  });
}

void Router::setPartitionList(const Server& server, const std::vector<int64_t> partition_list) {
  std::string key = getServerKey(server);
  servers_.withWLock([&](auto& servers) {
    if (servers.find(key) != servers.end()) {
      servers[key]->setPartitionList(partition_list);
    }
  });
}

folly::Optional<Server> Router::discover(const ClientOption& option) {
  auto server_list = getServerList(option.getServiceName(), option.getProtocol(), option.getShardId(),
                                   option.getShardType(), option.getDc());
  if (!server_list) {
    return folly::none;
  }

  return (*server_list)->selectServers(getOrCreateBalance(option));
}

folly::Optional<Server> Router::edgeDiscover(const ClientOption& option) {
  auto server_list = getServerList(option.getServiceName(), option.getProtocol(), option.getPartitionHash(),
                                   option.getShardType(), option.getDc());
  if (!server_list) {
    return folly::none;
  }

  return (*server_list)->selectServers(getOrCreateBalance(option));
}

std::unordered_map<int64_t, folly::Optional<Server>> Router::batchDiscover(const ClientOption& option,
                                                                           const std::vector<int64_t>& route_ids) {
  auto server_lists =
      getServerList(option.getServiceName(), option.getProtocol(), route_ids, option.getShardType(), option.getDc());
  std::unordered_map<int64_t, folly::Optional<Server>> result;
  std::unordered_map<uint64_t, folly::Optional<Server>> select_servers;
  for (auto& route_id : route_ids) {
    if (server_lists.find(route_id) == server_lists.end()) {
      result[route_id] = folly::none;
    } else {
      auto& server_list = server_lists[route_id];
      uint64_t hash_code = server_list->getHashCode();
      if (select_servers.find(hash_code) == select_servers.end()) {
        select_servers[hash_code] = server_list->selectServers(getOrCreateBalance(option));
      }
      result[route_id] = select_servers[hash_code];
    }
  }
  return result;
}

folly::Optional<std::shared_ptr<ServerList>> Router::getServerList(const std::string& service_name,
                                                                   const ServerProtocol& protocol, int64_t route_id,
                                                                   const ShardType& type, const std::string& dc) {
  getOrCreateDiscoverPuller(service_name);
  return db_->selectServers(service_name, protocol, route_id, type, dc);
}

std::unordered_map<int64_t, std::shared_ptr<ServerList>> Router::getServerList(const std::string& service_name,
                                                                               const ServerProtocol& protocol,
                                                                               const std::vector<int64_t>& route_ids,
                                                                               const ShardType& type,
                                                                               const std::string& dc) {
  getOrCreateDiscoverPuller(service_name);
  return db_->selectServers(service_name, protocol, route_ids, type, dc);
}

bool Router::unregisterServerSync(const Server server) {
  bool ret = false;
  servers_.withWLock([&](auto& servers) {
    if (servers.find(getServerKey(server)) == servers.end()) {
      return;
    }
    auto service_name = server.getServiceName();
    auto registry = getRegistry(service_name);
    if (registry == nullptr) {
      LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
      return;
    }
    ret = registry->unregisterServerSync(server);
    if (ret) {
      servers.erase(getServerKey(server));
    }
  });
  return ret;
}

void Router::unregisterServer(const Server server) {
  servers_.withWLock([&](auto& servers) {
    if (servers.find(getServerKey(server)) == servers.end()) {
      return;
    }

    auto service_name = server.getServiceName();
    auto registry = getRegistry(service_name);
    if (registry == nullptr) {
      LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
      return;
    }
    registry->unregisterServer(server);
    servers.erase(getServerKey(server));
  });
}

void Router::unregisterAll() {
  servers_.withWLock([&](auto& servers) {
    for (auto& s : servers) {
      auto service_name = s.second->getServer().getServiceName();
      auto registry = getRegistry(service_name);
      if (registry == nullptr) {
        LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
        continue;
      }
      registry->unregisterServer(s.second->getServer());
    }
    servers.clear();
  });
}

void Router::subscribeConfig(const std::string& service_name, NotifyConfig notify) {
  getOrCreateConfigPuller(service_name);
  auto registry = getRegistry(service_name);
  if (registry == nullptr) {
    LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
    return;
  }
  registry->subscribeConfig(service_name, std::move(notify));
}

void Router::unsubscribeConfig(const std::string& service_name) {
  getOrCreateConfigPuller(service_name);
  auto registry = getRegistry(service_name);
  if (registry == nullptr) {
    LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
    return;
  }
  registry->unsubscribeConfig(service_name);
}

void Router::subscribeConfigItem(const std::string& service_name, const std::string& key, NotifyConfigItem notify) {
  getOrCreateConfigPuller(service_name);
  auto registry = getRegistry(service_name);
  if (registry == nullptr) {
    LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
    return;
  }
  registry->subscribeConfigItem(service_name, key, std::move(notify));
}

void Router::unsubscribeConfigItem(const std::string& service_name, const std::string& key) {
  getOrCreateConfigPuller(service_name);
  auto registry = getRegistry(service_name);
  if (registry == nullptr) {
    LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
    return;
  }
  registry->unsubscribeConfigItem(service_name, key);
}

const std::string Router::getServerKey(const Server& server) {
  return folly::to<std::string>(server.getServiceName(), server.getHost(), folly::to<std::string>(server.getPort()));
}

const ServiceRouterConfig Router::getRouterConfig(const std::string& service_name) {
  getOrCreateConfigPuller(service_name);
  return db_->getRouterConfig(service_name);
}

const std::unordered_map<std::string, std::string> Router::getConfigs(const std::string& service_name) {
  getOrCreateConfigPuller(service_name);
  return db_->getConfig(service_name).getConfigs();
}

std::shared_ptr<ServiceInfoPuller<ServiceConfigPull>> Router::getOrCreateConfigPuller(const std::string& service_name) {
  auto puller = config_pullers_.withULockPtr([&](auto ulock) {
    if (ulock->find(service_name) != ulock->end()) {
      return ulock->at(service_name);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto registry = getRegistry(service_name);
    if (registry == nullptr) {
      LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
      return std::shared_ptr<ServiceInfoPuller<ServiceConfigPull>>(nullptr);
    }
    ServiceConfigPull config(db_.get(), registry.get());
    (*wlock)[service_name] =
        std::make_shared<ServiceInfoPuller<ServiceConfigPull>>(service_name, evb_, config, db_.get());
    return wlock->at(service_name);
  });
  return puller;
}

std::shared_ptr<ServiceInfoPuller<ServiceDiscoverPull>> Router::getOrCreateDiscoverPuller(
    const std::string& service_name) {
  auto puller = discover_pullers_.withULockPtr([&](auto ulock) {
    if (ulock->find(service_name) != ulock->end()) {
      return ulock->at(service_name);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto registry = getRegistry(service_name);
    if (registry == nullptr) {
      LOG(ERROR) << "Registry of " << service_name << " is nullptr!";
      return std::shared_ptr<ServiceInfoPuller<ServiceDiscoverPull>>(nullptr);
    }
    ServiceDiscoverPull config(db_.get(), registry.get());
    VLOG(4) << "Create service discover puller, service_name:" << service_name;
    (*wlock)[service_name] =
        std::make_shared<ServiceInfoPuller<ServiceDiscoverPull>>(service_name, evb_, config, db_.get());
    return wlock->at(service_name);
  });
  return puller;
}

std::shared_ptr<LoadBalanceInterface> Router::getOrCreateBalance(const ClientOption& option) {
  LoadBalanceMethod method = option.getLoadBalance();
  std::string method_str = toStringLoadBalanceMethod(method);
  std::string user_balance = option.getUserBalance();
  std::string protocol = toStringServerProtocol(option.getProtocol());

  // hash : method : user_balance : service_name : protocol
  uint64_t balance_id = CityHash64WithSeed(method_str.data(), method_str.size(), 0);
  balance_id = CityHash64WithSeed(option.getServiceName().data(), option.getServiceName().size(), balance_id);
  balance_id = CityHash64WithSeed(protocol.data(), protocol.size(), balance_id);
  if (method == LoadBalanceMethod::USER_DEFINED && !user_balance.empty()) {
    balance_id = CityHash64WithSeed(user_balance.data(), user_balance.size(), balance_id);
  }

  auto balance = balancers_.withULockPtr([balance_id, &method, &option, this](auto ulock) {
    if (ulock->find(balance_id) != ulock->end()) {
      return ulock->at(balance_id);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    switch (method) {
      case LoadBalanceMethod::ROUNDROBIN:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceRoundrobin>(this);
        break;
      case LoadBalanceMethod::LOCALFIRST:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceLocalFirst>(this, option.getLocalFirstConfig());
        break;
      case LoadBalanceMethod::CONFIGURABLE_WEIGHT:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceConfigurableWeight>(this);
        break;
      case LoadBalanceMethod::IPRANGEFIRST:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceIpRangeFirst>(this, option.getLocalFirstConfig());
        break;
      case LoadBalanceMethod::STATIC_WEIGHT:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceStaticWeight>(this);
        break;
      case LoadBalanceMethod::IDCFIRST:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceIdcFirst>(this, option.getIdc());
        break;
      default:
        (*wlock)[balance_id] = std::make_shared<LoadBalanceRandom>(this);
        break;
    }
    return wlock->at(balance_id);
  });
  return balance;
}

void Router::getAllRegistryName(std::vector<std::string>* registry_names) {
  registries_.withRLock([&](const auto& registries) {
    for (auto& registry : registries) {
      registry_names->emplace_back(registry.first);
    }
  });
}

void Router::setServiceRegistry(const std::string& service_name, std::shared_ptr<RegistryInterface> registry) {
  registries_.withWLock([&](auto& wlock) { wlock[service_name] = registry; });
}

std::shared_ptr<RegistryInterface> Router::createRegistry(const std::string& service_name, RegistryType type,
                                                          const std::string& address) {
  auto registry = registries_.withULockPtr([&](auto ulock) {
    if (ulock->find(service_name) != ulock->end()) {
      return ulock->at(service_name);
    }
    std::shared_ptr<RegistryInterface> registry = nullptr;
    switch (type) {
      case RegistryType::CONSUL:
        registry = std::make_shared<ConsulRegistry>(evb_, callback_thread_pool_, address);
        break;
      case RegistryType::AGENT:
      default:
        LOG(ERROR) << "Unimplement RegistryType!";
    }
    if (registry != nullptr) {
      auto wlock = ulock.moveFromUpgradeToWrite();
      (*wlock)[service_name] = registry;
    }
    return registry;
  });
  return registry;
}

std::shared_ptr<RegistryInterface> Router::getRegistry(const std::string& service_name) {
  std::shared_ptr<RegistryInterface> registry = registries_.withULockPtr([&](auto ulock) {
    if (ulock->find(service_name) != ulock->end()) {
      return ulock->at(service_name);
    }
    return std::shared_ptr<RegistryInterface>(nullptr);
  });
  return registry;
}

std::shared_ptr<RegistryInterface> Router::getOrCreateRegistry(const std::string& service_name) {
  auto registry = getRegistry(service_name);
  if (registry == nullptr) {
    RegistryType registry_type = RegistryType::CONSUL;
    auto op_registry_type = stringToRegistryType(FLAGS_global_registry_type);
    if (op_registry_type.hasValue()) {
      registry_type = op_registry_type.value();
    } else {
      LOG(ERROR) << "Unknown registry type: " << FLAGS_global_registry_type << "! Use "
                 << toStringRegistryType(RegistryType::CONSUL) << " as default type";
    }

    if (FLAGS_global_registry_addresses == "") {
      FLAGS_global_registry_addresses = FLAGS_router_consul_addresses;
    }
    registry = createRegistry(service_name, registry_type, FLAGS_global_registry_addresses);
    LOG(WARNING) << "The registry for " << service_name << " is created implicitly. It should be created explicitly "
                 << "using service_router::createServiceRegistry or service_router::createTargetServiceReigstry.";
  }
  return registry;
}

void Router::stop() {
  discover_pullers_.withWLock([](auto& discover_pullers) { discover_pullers.clear(); });
  config_pullers_.withWLock([](auto& config_pullers) { config_pullers.clear(); });
  callback_thread_pool_->stop();
}

static void waitForConfig(const std::string& name, int wait_milliseconds) {
  auto router = Router::getInstance();
  // 等待对应的配置的获取
  if (wait_milliseconds <= 0) {
    router->waitForConfig(name);
  } else {
    router->waitUntilConfig(name, wait_milliseconds);
  }
}

void createServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address) {
  auto router = Router::getInstance();
  router->createServiceRegistry(service_name, type, address);
}

void createTargetServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address) {
  auto router = Router::getInstance();
  router->createTargetServiceRegistry(service_name, type, address);
}

bool registerServerSync(const Server& server, int wait_milliseconds) {
  auto router = Router::getInstance();
  VLOG(2) << "Wait get service config.";
  waitForConfig(server.getServiceName(), wait_milliseconds);
  bool ret = router->registerServerSync(server);
  if (!ret) {
    VLOG(2) << "Register server failed.";
  } else {
    VLOG(2) << "Register server success.";
  }
  return ret;
}

void registerServer(const Server& server, int wait_milliseconds) {
  auto router = Router::getInstance();
  VLOG(2) << "Wait get service config.";
  waitForConfig(server.getServiceName(), wait_milliseconds);
  router->registerServer(server);
  VLOG(2) << "Register server success.";
}

bool unregisterServerSync(const Server& server) {
  auto router = Router::getInstance();
  return router->unregisterServerSync(server);
}

void unregisterServer(const Server& server) {
  auto router = Router::getInstance();
  router->unregisterServer(server);
}

void unregisterAll() {
  auto router = Router::getInstance();
  router->unregisterAll();
}

void stop() {
  auto router = Router::getInstance();
  router->stop();
}

void subscribeConfig(const std::string& service_name, NotifyConfig notify) {
  auto router = Router::getInstance();
  router->subscribeConfig(service_name, std::move(notify));
}

void unsubscribeConfig(const std::string& service_name) {
  auto router = Router::getInstance();
  router->unsubscribeConfig(service_name);
}

void subscribeConfigItem(const std::string& service_name, const std::string& key, NotifyConfigItem notify) {
  auto router = Router::getInstance();
  router->subscribeConfigItem(service_name, key, std::move(notify));
}

void unsubscribeConfigItem(const std::string& service_name, const std::string& key) {
  auto router = Router::getInstance();
  router->unsubscribeConfigItem(service_name, key);
}

const std::unordered_map<std::string, std::string> getConfigs(const std::string& service_name, int wait_milliseconds) {
  auto router = Router::getInstance();
  waitForConfig(service_name, wait_milliseconds);
  return router->getConfigs(service_name);
}

bool serviceClient(ServerAddress* address, const ClientOption& option, int wait_milliseconds) {
  // 手动指定服务地址，不进行服务发现
  ServerAddress target_addr = option.getTargetServerAddress();
  if (target_addr.getPort() != 0 && !target_addr.getHost().empty()) {
    *address = target_addr;
    return true;
  }

  auto router = Router::getInstance();
  // 等待对应的配置的获取
  if (wait_milliseconds <= 0) {
    router->waitForDiscover(option.getServiceName());
  } else {
    router->waitUntilDiscover(option.getServiceName(), wait_milliseconds);
  }

  folly::Optional<Server> server = folly::none;
  if (option.getRouteToEdgeNode()) {
    server = router->edgeDiscover(option);
  }
  if (!server) {
    server = router->discover(option);
  }
  if (!server) {
    std::unordered_map<std::string, std::string> select_addr_tags = {
        {ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR, ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR_VAL_NONE}};
    auto select_counter = metrics::Metrics::getInstance()->buildCounter(
        ROUTER_METRICS_MODULE_NAME, ROUTER_METRICS_SELECT_ADDRESS, select_addr_tags);
    select_counter->inc(1);
    return false;
  }

  ServerAddress addr;
  addr.setHost((*server).getHost());
  addr.setPort((*server).getPort());
  *address = addr;
  std::unordered_map<std::string, std::string> select_addr_tags = {
      {ROUTER_METRICS_SELECT_ADDRESS_TAGS_ADDR, folly::to<std::string>(addr.getHost(), ":", addr.getPort())}};
  auto select_counter = metrics::Metrics::getInstance()->buildCounter(ROUTER_METRICS_MODULE_NAME,
                                                                      ROUTER_METRICS_SELECT_ADDRESS, select_addr_tags);
  select_counter->inc(1);
  return true;
}

}  // namespace service_router
