/*
 * Copyright 2020 Weibo Inc.
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

#include "registry.h"

#include "folly/portability/GFlags.h"

namespace service_router {

DEFINE_string(router_consul_addresses, "127.0.0.1:8500", "Service router consul host list");
DEFINE_string(router_consul_token, "7e4e0ceb-feff-749d-3e58-69b4697e191a", "Service router consul token");
DEFINE_int32(router_consul_failback_interval, 1, "Service router consul fallback interval call");

constexpr static char CONSUL_REGISTRY_KV_KEY_SPLIT[] = ":";
constexpr static char CONSUL_REGISTRY_KV_PREFIX[] = "ads-core/services/";
constexpr static char CONSUL_REGISTRY_KV_CONFIG[] = "/config/";
constexpr static char CONSUL_REGISTRY_KV_CONFIG_ROUTER[] = "router";
constexpr static char CONSUL_REGISTRY_KV_CONFIG_CONFIGS[] = "configs/";
constexpr static char CONSUL_REGISTRY_KV_NODES[] = "/nodes/";

// metrics
constexpr static char CONSUL_REGISTRY_MODULE_NAME[] = "service_router";
constexpr static char CONSUL_REGISTRY_FALLBACK_METRIC_NAME[] = "fallback";
constexpr static char CONSUL_REGISTRY_SERVICE_REG_COUNT_METRIC_NAME[] = "service_reg_count";
constexpr static char CONSUL_REGISTRY_SERVICE_REG_FALLBACK_METRIC_NAME[] = "service_reg_fallback";
constexpr static char CONSUL_REGISTRY_SERVICE_GET_COUNT_METRIC_NAME[] = "service_get_count";
constexpr static char CONSUL_REGISTRY_SERVICE_GET_FALLBACK_METRIC_NAME[] = "service_get_fallback";
constexpr static char CONSUL_REGISTRY_CONFIG_GET_COUNT_METRIC_NAME[] = "config_get_count";
constexpr static char CONSUL_REGISTRY_CONFIG_GET_FALLBACK_METRIC_NAME[] = "config_get_fallback";

using AsyncConsulResponse = service_framework::consul::RawClient::HttpResponse;

ConsulRegistry::ConsulRegistry(folly::EventBase* evb, std::shared_ptr<folly::CPUThreadPoolExecutor> thread_pool,
                               const std::string& address)
    : evb_(evb), callback_thread_pool_(thread_pool), address_(address) {
  if (address_.empty()) {
    address_ = FLAGS_router_consul_addresses;
  }
  std::vector<std::string> addresses;
  folly::split(',', address_, addresses);
  if (addresses.empty()) {
    LOG(FATAL) << "Service router consul list is empty.";
  }

  for (auto& address : addresses) {
    std::string host;
    uint16_t port = 0;
    folly::split(':', address, host, port);
    kvs_.withWLock([host, port](auto& kvs) {
      service_framework::consul::KVClient client(host, port);
      kvs.push_back(client);
    });
  }
  schedule_ = folly::AsyncTimeout::make(*evb_, [&]() noexcept {
    fallback();
    schedule_->scheduleTimeout(std::chrono::seconds(FLAGS_router_consul_failback_interval));
  });

  evb_->runInEventBaseThread(
      [&]() { schedule_->scheduleTimeout(std::chrono::seconds(FLAGS_router_consul_failback_interval)); });

  auto metrics = metrics::Metrics::getInstance();
  ser_reg_count_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_SERVICE_REG_COUNT_METRIC_NAME);
  ser_reg_fail_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_SERVICE_REG_FALLBACK_METRIC_NAME);
  ser_get_count_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_SERVICE_GET_COUNT_METRIC_NAME);
  ser_get_fail_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_SERVICE_GET_FALLBACK_METRIC_NAME);
  conf_get_count_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_CONFIG_GET_COUNT_METRIC_NAME);
  conf_get_fail_ = metrics->buildCounter(CONSUL_REGISTRY_MODULE_NAME, CONSUL_REGISTRY_CONFIG_GET_FALLBACK_METRIC_NAME);
}

ConsulRegistry::~ConsulRegistry() { evb_ = nullptr; }

service_framework::consul::KVClient ConsulRegistry::getKvClient(bool change) {
  return kvs_.withULockPtr([change](auto ulock) {
    if (!change) {
      return ulock->at(0);
    }
    auto wlock = ulock.moveFromUpgradeToWrite();
    std::random_shuffle(wlock->begin(), wlock->end());
    return wlock->at(0);
  });
}

void ConsulRegistry::fallback() {
  VLOG(5) << "Start fallback.";
  // 处理失败的 register 操作
  auto fallbacks = fallbacks_.withWLock([](auto& fallbacks) {
    auto res = fallbacks;
    fallbacks.clear();
    return res;
  });
  if (!fallbacks.empty()) {
    getKvClient(true);
  }
  for (auto& t : fallbacks) {
    FallbackType type = t.second.getType();
    switch (type) {
      case FallbackType::REGISTER:
        registerServer(t.second.getServer());
        break;
      case FallbackType::UNREGISTER:
        unregisterServer(t.second.getServer());
        break;
      case FallbackType::DISCOVER:
        discover(t.second.getServer().getServiceName());
        break;
      case FallbackType::GET_CONFIG:
        getConfig(t.second.getServer().getServiceName());
        break;
    }
  }
}

bool ConsulRegistry::registerServerSync(const Server& updateServer) {
  std::string json;
  ser_reg_count_->inc(1);
  if (!common::toJson(&json, updateServer.serialize())) {
    VLOG(10) << "serialize server json fail.";
    return false;
  }

  if (!checkServerValid(updateServer)) {
    return false;
  }

  auto thread_pool = callback_thread_pool_.lock();
  if (!thread_pool) {
    return false;
  }
  auto result = getKvClient().setValue(ConsulNode::getNodePath(updateServer), json, FLAGS_router_consul_token);
  return std::move(result).via(thread_pool.get()).thenValue([this, updateServer](const AsyncConsulResponse& response) {
    auto res = getKvClient().processBool(response);
    if (res && res->getValue()) {
      VLOG(4) << "Register server success, result:" << *res;
      return true;
    } else {
      ser_reg_fail_->inc(1);
      std::string nodeName = ConsulNode::getNodeName(updateServer);
      fallbacks_.withWLock([this, &updateServer](auto& fallbacks) {
        // 此处保证唯一 server 在注册，但是永远是最终的状态
        FallBackDetail fallback(updateServer, FallbackType::REGISTER);
        fallbacks[ConsulNode::getNodeName(updateServer)] = fallback;
      });
      VLOG(4) << "Register server fail, result:" << *res << " server:" << updateServer;
      return false;
    }
  }).get();
}


void ConsulRegistry::registerServer(const Server& updateServer) {
  std::string json;
  ser_reg_count_->inc(1);
  if (!common::toJson(&json, updateServer.serialize())) {
    VLOG(10) << "serialize server json fail.";
    return;
  }

  if (!checkServerValid(updateServer)) {
    return;
  }

  auto thread_pool = callback_thread_pool_.lock();
  if (!thread_pool) {
    return;
  }
  auto result = getKvClient().setValue(ConsulNode::getNodePath(updateServer), json, FLAGS_router_consul_token);
  result.via(thread_pool.get()).thenValue([this, updateServer](const AsyncConsulResponse& response) {
    auto res = getKvClient().processBool(response);
    if (res && (*res).getValue()) {
      VLOG(4) << "Register server success, result:" << *res;
    } else {
      ser_reg_fail_->inc(1);
      std::string nodeName = ConsulNode::getNodeName(updateServer);
      fallbacks_.withWLock([this, &updateServer](auto& fallbacks) {
        // 此处保证唯一 server 在注册，但是永远是最终的状态
        FallBackDetail fallback(updateServer, FallbackType::REGISTER);
        fallbacks[ConsulNode::getNodeName(updateServer)] = fallback;
      });
      VLOG(4) << "Register server fail, result:" << *res << " server:" << updateServer;
    }
  });
}

bool ConsulRegistry::unregisterServerSync(const Server& updateServer) {
  if (!checkServerValid(updateServer)) {
    return false;
  }

  auto thread_pool = callback_thread_pool_.lock();
  if (!thread_pool) {
    return false;
  }
  auto result = getKvClient().deleteValue(ConsulNode::getNodePath(updateServer));
  return std::move(result).via(thread_pool.get()).thenValue([this, updateServer](const AsyncConsulResponse& response) {
    auto res = getKvClient().processBool(response);
    if (res && (*res).getValue()) {
      VLOG(4) << "Unregister server success, result:" << *res;
      return true;
    } else {
      VLOG(4) << "Unregister server fail, result:" << *res;
      std::string nodeName = ConsulNode::getNodeName(updateServer);
      fallbacks_.withWLock([this, &updateServer](auto& fallbacks) {
        // 此处保证唯一 server 在注册，但是永远是最终的状态
        FallBackDetail fallback(updateServer, FallbackType::UNREGISTER);
        fallbacks[ConsulNode::getNodeName(updateServer)] = fallback;
      });
      return false;
    }
  }).get();
}

void ConsulRegistry::unregisterServer(const Server& updateServer) {
  if (!checkServerValid(updateServer)) {
    return;
  }

  auto thread_pool = callback_thread_pool_.lock();
  if (!thread_pool) {
    return;
  }
  auto result = getKvClient().deleteValue(ConsulNode::getNodePath(updateServer));
  result.via(thread_pool.get()).thenValue([this, updateServer](const AsyncConsulResponse& response) {
    auto res = getKvClient().processBool(response);
    if (res && (*res).getValue()) {
      VLOG(4) << "Unregister server success, result:" << *res;
    } else {
      VLOG(4) << "Unregister server fail, result:" << *res;
      std::string nodeName = ConsulNode::getNodeName(updateServer);
      fallbacks_.withWLock([this, &updateServer](auto& fallbacks) {
        // 此处保证唯一 server 在注册，但是永远是最终的状态
        FallBackDetail fallback(updateServer, FallbackType::UNREGISTER);
        fallbacks[ConsulNode::getNodeName(updateServer)] = fallback;
      });
    }
  });
}

void ConsulRegistry::available(const Server& server) {
  Server updateServer = server;
  updateServer.setStatus(ServerStatus::AVAILABLE);
  registerServer(updateServer);
}

void ConsulRegistry::unavailable(const Server& server) {
  Server updateServer = server;
  updateServer.setStatus(ServerStatus::UNAVAILABLE);
  registerServer(updateServer);
}

void ConsulRegistry::subscribe(const std::string& service_name, NotifyService notify) {
  service_subscribers_.withWLock([&](auto& subscribes) {
    using Notify = NotifyDetail<NotifyService, std::vector<Server> >;
    std::shared_ptr<Notify> detail = std::make_shared<Notify>(service_name, std::move(notify));
    subscribes[service_name] = detail;
  });
}

void ConsulRegistry::subscribeConfigItem(const std::string& service_name, const std::string& config_key,
                                         NotifyConfigItem notify) {
  config_item_subscribers_.withWLock([&](auto& subscribes) {
    std::string key = folly::to<std::string>(service_name, config_key);
    std::shared_ptr<NotifyConfigDetail> detail =
        std::make_shared<NotifyConfigDetail>(service_name, config_key, std::move(notify));
    subscribes[key] = detail;
  });
}

void ConsulRegistry::subscribeConfig(const std::string& service_name, NotifyConfig notify) {
  config_subscribers_.withWLock([&](auto& subscribes) {
    using Notify = NotifyDetail<NotifyConfig, ServiceConfig>;
    std::shared_ptr<Notify> detail = std::make_shared<Notify>(service_name, std::move(notify));
    subscribes[service_name] = detail;
  });
}

void ConsulRegistry::unsubscribe(const std::string& service_name) {
  service_subscribers_.withWLock([&](auto& subscribes) {
    if (subscribes.find(service_name) != subscribes.end()) {
      subscribes.erase(service_name);
    }
  });
}

void ConsulRegistry::unsubscribeConfigItem(const std::string& service_name, const std::string& config_key) {
  config_item_subscribers_.withWLock([&](auto& subscribes) {
    std::string key = folly::to<std::string>(service_name, config_key);
    if (subscribes.find(key) != subscribes.end()) {
      subscribes.erase(key);
    }
  });
}

void ConsulRegistry::unsubscribeConfig(const std::string& service_name) {
  config_subscribers_.withWLock([&](auto& subscribes) {
    if (subscribes.find(service_name) != subscribes.end()) {
      subscribes.erase(service_name);
    }
  });
}

void ConsulRegistry::discover(const std::string& service_name) {
  ser_get_count_->inc(1);
  bool use_static = use_static_.load();
  if (!use_static) {
    Server server;
    server.setServiceName(service_name);
    auto thread_pool = callback_thread_pool_.lock();
    if (!thread_pool) {
      return;
    }
    auto result = getKvClient().getValues(ConsulNode::getNodesPath(server));
    result.via(thread_pool.get()).thenValue([this, server](const AsyncConsulResponse& response) {
      auto res = getKvClient().processValues(response);
      std::string path = ConsulNode::getNodesPath(server);
      if (res) {
        VLOG(4) << "Discover server success, result:" << *res;
        processDiscover(server, *res);
      } else {
        ser_get_fail_->inc(1);
        VLOG(4) << "Discover server fail, server:" << server;
        fallbacks_.withWLock([this, &server, &path](auto& fallbacks) {
          FallBackDetail fallback(server, FallbackType::DISCOVER);
          fallbacks[path] = fallback;
        });
      }
    });
  } else {
    std::vector<Server> servers = getStaticNodes(service_name);
    if (!servers.empty()) {
      notifyDiscover(service_name, servers);
    }
  }
}

void ConsulRegistry::notifyDiscover(const std::string& service_name, const std::vector<Server>& servers) {
  service_subscribers_.withRLock([&](auto& subscribes) {
    if (subscribes.find(service_name) != subscribes.end()) {
      auto subscribe = subscribes.at(service_name);
      subscribe->notify(servers);
    }
  });
}

void ConsulRegistry::processDiscover(const Server& server, const ConsulValues& values) {
  std::string index_path = ConsulNode::getNodesPath(server);
  bool is_update = checkUpdate(index_path, values.getConsulIndex());
  if (values.getValue().empty() || !is_update) {
    VLOG(4) << "Discover is empty or not need update";
    return;
  }

  std::vector<Server> servers;
  for (auto& t : values.getValue()) {
    std::string vkey = t.getKey();
    std::string vval = proxygen::Base64::decode(t.getValue(), 0);
    VLOG(4) << "key:" << vkey << " value:" << vval;
    folly::dynamic dy_server;
    if (!common::fromJson(&dy_server, vval)) {
      VLOG(4) << "Json parse is fail, vval:" << vval;
      continue;
    }

    Server target_server;
    if (!target_server.deserialize(dy_server)) {
      VLOG(4) << "Json to object is fail, vval:" << vval;
      continue;
    }
    servers.push_back(target_server);
  }

  notifyDiscover(server.getServiceName(), servers);
}

void ConsulRegistry::getConfig(const std::string& service_name) {
  conf_get_count_->inc(1);
  Server server;
  server.setServiceName(service_name);
  auto thread_pool = callback_thread_pool_.lock();
  if (!thread_pool) {
    return;
  }
  auto result = getKvClient().getValues(ConsulNode::getConfigPath(server));
  result.via(thread_pool.get()).thenValue([this, server](const AsyncConsulResponse& response) {
    auto res = getKvClient().processValues(response);
    std::string path = ConsulNode::getConfigPath(server);
    if (res) {
      VLOG(4) << "Get config success, result:" << *res;
      processConfig(server, *res);
    } else {
      conf_get_fail_->inc(1);
      VLOG(4) << "Get config fail, server:" << server;
      fallbacks_.withWLock([this, &server, &path](auto& fallbacks) {
        FallBackDetail fallback(server, FallbackType::GET_CONFIG);
        fallbacks[path] = fallback;
      });
    }
  });
}

void ConsulRegistry::processConfig(const Server& server, const ConsulValues& values) {
  std::string index_path = ConsulNode::getConfigPath(server);
  bool is_update = checkUpdate(index_path, values.getConsulIndex());
  if (values.getValue().empty()) {
    fallbacks_.withWLock([this, &server](auto& fallbacks) {
      FallBackDetail fallback(server, FallbackType::GET_CONFIG);
      fallbacks[ConsulNode::getConfigPath(server)] = fallback;
    });
    VLOG(4) << "Service config is empty, service info: " << server;
    return;
  }
  if (!is_update) {
    return;
  }

  ServiceConfig service_config;
  std::unordered_map<std::string, std::string> configs;
  for (auto& t : values.getValue()) {
    std::string vkey = t.getKey();
    std::string vval = proxygen::Base64::decode(t.getValue(), 0);
    VLOG(4) << "key:" << vkey << " value:" << vval;
    if (vkey.find(ConsulNode::getRouterConfigPath(server)) != std::string::npos) {  // router
      folly::dynamic dy_router_config;
      if (!common::fromJson(&dy_router_config, vval)) {
        VLOG(4) << "Json parse is fail, vval:" << vval;
        continue;
      }
      ServiceRouterConfig router_config;
      if (!router_config.deserialize(dy_router_config)) {
        VLOG(4) << "Json to object is fail, vval:" << vval;
        continue;
      }
      service_config.setRouter(router_config);
      continue;
    }

    if (vkey.find(ConsulNode::getConfigsPath(server)) != std::string::npos) {  // configs
      std::string config_key = vkey.substr(ConsulNode::getConfigsPath(server).size());
      configs[config_key] = vval;
      config_item_subscribers_.withRLock([&](auto& subscribes) {
        std::string service_name = server.getServiceName();
        std::string key = folly::to<std::string>(service_name, config_key);
        if (subscribes.find(key) != subscribes.end()) {
          auto subscribe = subscribes.at(key);
          subscribe->notify(vval);
        }
      });
    }
  }
  service_config.setConfigs(configs);

  config_subscribers_.withRLock([&](auto& subscribes) {
    std::string service_name = server.getServiceName();
    if (subscribes.find(service_name) != subscribes.end()) {
      auto subscribe = subscribes.at(service_name);
      subscribe->notify(service_config);
    }
  });
}

bool ConsulRegistry::checkUpdate(const std::string& key, uint64_t index) {
  return last_indexes_.withULockPtr([&](auto ulock) {
    uint64_t last_index = 0;
    if (ulock->find(key) != ulock->end()) {
      last_index = ulock->at(key);
    }

    if (index == last_index) {
      return false;
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    (*wlock)[key] = index;
    return true;
  });
}

bool ConsulRegistry::checkServerValid(const Server& server) {
  if (server.getServiceName().empty()) {
    VLOG(4) << "Register server fail, given service name is empty, server:" << server;
    return false;
  }

  return true;
}

void ConsulRegistry::getSubscribedService(std::vector<std::string>* services) {
  service_subscribers_.withRLock([&](const auto& subscribers) {
    for (auto& subscriber : subscribers) {
      services->emplace_back(subscriber.first);
    }
  });
}

void FileRegistry::registerServersFromFile(const std::string& service_name, const std::string& file_path) {
  file_readers_.withWLock([&](auto& file_readers) {
    file_readers[service_name] = std::make_shared<FileServerReader>(service_name, file_path);
  });
}

void FileRegistry::discover(const std::string& service_name) {
  std::vector<Server> server_list;
  auto ret = file_readers_.withRLock([&service_name, &server_list](auto& rlock) {
    if (rlock.find(service_name) == rlock.end()) {
      return false;
    }
    auto file_reader_ptr = rlock.at(service_name);
    return file_reader_ptr->getServers(&server_list);
  });

  if (!ret || server_list.empty()) {
    return;
  }

  // notify
  service_subscribers_.withRLock([&service_name, &server_list](auto& service_subscribers) {
    if (service_subscribers.find(service_name) != service_subscribers.end()) {
      auto& service_subscriber = service_subscribers.at(service_name);
      service_subscriber->notify(server_list);
    }
  });
}

void FileRegistry::subscribe(const std::string& service_name, NotifyService notify) {
  service_subscribers_.withWLock([&](auto& subscribes) {
    using Notify = NotifyDetail<NotifyService, std::vector<Server> >;
    std::shared_ptr<Notify> detail = std::make_shared<Notify>(service_name, std::move(notify));
    subscribes[service_name] = detail;
  });
}

void FileRegistry::unsubscribe(const std::string& service_name) {
  service_subscribers_.withWLock([&](auto& service_subscribers) {
    if (service_subscribers.find(service_name) != service_subscribers.end()) {
      service_subscribers.erase(service_name);
    }
  });
}

void FileRegistry::subscribeConfig(const std::string& service_name, NotifyConfig notify) {
  config_subscribers_.withWLock([&](auto& subscribers) {
    using Notify = NotifyDetail<NotifyConfig, ServiceConfig>;
    std::shared_ptr<Notify> detail = std::make_shared<Notify>(service_name, std::move(notify));
    subscribers[service_name] = detail;
  });
}

void FileRegistry::unsubscribeConfig(const std::string& service_name) {
  config_subscribers_.withWLock([&](auto& subscribers) {
    if (subscribers.find(service_name) != subscribers.end()) {
      subscribers.erase(service_name);
    }
  });
}

void FileRegistry::getConfig(const std::string& service_name) {
  config_subscribers_.withRLock([&](auto& config_subscribers) {
    if (config_subscribers.find(service_name) != config_subscribers.end()) {
      auto config_subscriber = config_subscribers.at(service_name);
      config_subscriber->notify(service_config_);
    }
  });
}

bool FileRegistry::FileServerReader::getServers(std::vector<service_router::Server>* server_list) {
  if (!server_list || !checkModified()) {
    return false;
  }
  std::ifstream input;
  input.open(file_path_);
  if (!input.is_open()) {
    LOG(ERROR) << "Fail to open file " << file_path_;
    return false;
  }
  SCOPE_EXIT { input.close(); };
  std::string line;
  while (!input.eof()) {
    std::getline(input, line);
    // remove all space char
    auto it = std::remove_if(line.begin(), line.end(), ::isspace);
    line.erase(it, line.end());
    if (line.empty() || line.at(0) == '#') {
      continue;
    }
    Server server;
    if (!parseServerFromString(&server, line)) {
      LOG(ERROR) << "Illegal line:" << line;
      continue;
    }
    server_list->emplace_back(std::move(server));
  }
  LOG(INFO) << file_path_ << ": Got " << server_list->size() << " servers.";
  return true;
}

bool FileRegistry::FileServerReader::checkModified() {
  struct stat file_info;
  if (stat(file_path_.c_str(), &file_info) == 0) {
    time_t mod_time = file_info.st_mtime;
    if (mod_time != last_modified_time_) {
      last_modified_time_ = mod_time;
      return true;
    }
  }
  return false;
}

bool FileRegistry::FileServerReader::parseServerFromString(service_router::Server* server, const std::string& line) {
  if (line.empty() || !server) {
    return false;
  }
  std::vector<std::string> line_vec;
  folly::split(',', line, line_vec);
  // protocol,ip:port,dc,weight
  // thrift,192.168.10.10:2020,aliyun,2
  if (line_vec.size() != 4) {
    return false;
  }
  std::string ip;
  uint16_t port;
  if (!splitIpPort(&ip, &port, line_vec[1])) {
    return false;
  }
  auto protocol = stringToServerProtocol(line_vec[0]);
  if (!protocol) {
    LOG(ERROR) << "Unsupported protocol:" << line_vec[0];
    return false;
  }
  server->setServiceName(service_name_);
  server->setHost(ip);
  server->setPort(port);
  server->setIdc(line_vec[2]);
  server->setProtocol(*protocol);
  server->setStatus(ServerStatus::AVAILABLE);
  server->setUpdateTime(0);
  auto weight = folly::tryTo<int32_t>(line_vec[3]);
  if (!weight.hasValue()) {
    server->setWeight(1);
  } else {
    server->setWeight(weight.value());
  }
  return true;
}

bool FileRegistry::FileServerReader::splitIpPort(std::string* ip, uint16_t* port, const std::string& str) {
  if (str.empty()) {
    return false;
  }
  std::vector<std::string> vec;
  folly::split(':', str, vec);
  if (vec.size() != 2) {
    return false;
  }
  auto port_op = folly::tryTo<uint16_t>(vec[1]);
  if (!port_op.hasValue()) {
    return false;
  }
  *ip = vec[0];
  *port = port_op.value();
  return true;
}

const std::string ConsulNode::getNodeName(const Server& server) {
  return folly::to<std::string>(toStringServerProtocol(server.getProtocol()), CONSUL_REGISTRY_KV_KEY_SPLIT,
                                server.getHost(), CONSUL_REGISTRY_KV_KEY_SPLIT,
                                folly::to<std::string>(server.getPort()));
}

const std::string ConsulNode::getNodePath(const Server& server) { return getNodesPath(server) + getNodeName(server); }

const std::string ConsulNode::getNodesPath(const Server& server) {
  return CONSUL_REGISTRY_KV_PREFIX + server.getServiceName() + CONSUL_REGISTRY_KV_NODES;
}

const std::string ConsulNode::getConfigsPath(const Server& server) {
  return getConfigPath(server) + CONSUL_REGISTRY_KV_CONFIG_CONFIGS;
}

const std::string ConsulNode::getRouterConfigPath(const Server& server) {
  return getConfigPath(server) + CONSUL_REGISTRY_KV_CONFIG_ROUTER;
}

const std::string ConsulNode::getConfigPath(const Server& server) {
  return CONSUL_REGISTRY_KV_PREFIX + server.getServiceName() + CONSUL_REGISTRY_KV_CONFIG;
}

const std::string ConsulNode::getServices() { return CONSUL_REGISTRY_KV_PREFIX; }

}  // namespace service_router
