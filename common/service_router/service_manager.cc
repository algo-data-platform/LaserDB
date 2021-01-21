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
 */

#include <iomanip>

#include "folly/init/Init.h"
#include "folly/portability/GFlags.h"

#include "common/console/table.h"
#include "router.h"

constexpr static char CONSUL_KV_KEY_SPLIT = '/';

DEFINE_string(service_name, "", "service name");
DEFINE_int32(timeout, 500, "Get service info timeout");
DEFINE_int32(config_timeout, 20, "Get service info timeout");

#define COMMAND_FAIL 1
#define COMMAND_SUCCESS 0

using ConsulNode = service_router::ConsulNode;
using ConsoleTable = common::console::ConsoleTable;
using ConsoleTableCell = common::console::ConsoleTableCell;
using ConsoleTableColor = common::console::ConsoleTableColor;

namespace service_router {
DECLARE_string(router_consul_addresses);
DECLARE_string(router_consul_token);
}

class ServiceManager {
 public:
  ServiceManager() {
    // 复用 FLAGS_router_consul_addresses
    std::vector<std::string> addresses;
    folly::split(',', service_router::FLAGS_router_consul_addresses, addresses);
    if (addresses.empty()) {
      LOG(FATAL) << "Service router consul list is empty.";
    }
    std::string host;
    uint16_t port = 0;
    folly::split(':', addresses[0], host, port);
    kv_ = std::make_shared<service_framework::consul::KVClient>(host, port);
  }
  ~ServiceManager() = default;
  ServiceManager(const ServiceManager&) = default;
  ServiceManager(ServiceManager&&) = default;
  ServiceManager& operator=(const ServiceManager&) = default;

  bool createRouterConfig(const service_router::Server& server) {
    service_router::ServiceRouterConfig config;
    folly::dynamic dy_router_config = config.serialize();
    std::string value;
    if (!common::toJson(&value, dy_router_config)) {
      return false;
    }
    std::string key = ConsulNode::getRouterConfigPath(server);
    auto result = kv_->setValueSync(key, value, service_router::FLAGS_router_consul_token);
    if (result) {
      return (*result).getValue();
    }
    return false;
  }

  bool createConfigs(const service_router::Server& server) {
    std::string key = ConsulNode::getConfigsPath(server);
    auto result = kv_->setValueSync(key, "", service_router::FLAGS_router_consul_token);
    if (result) {
      return (*result).getValue();
    }
    return false;
  }

  bool getServices(std::set<std::string>* service_list) {
    std::string key = ConsulNode::getServices();
    auto result = kv_->getValuesSync(key);
    if (!result) {
      return false;
    }
    auto values = (*result).getValue();
    if (values.empty()) {
      return false;
    }

    for (auto& t : values) {
      std::string vkey = t.getKey().substr(key.size());
      std::vector<std::string> consul_keys;
      folly::split(CONSUL_KV_KEY_SPLIT, vkey, consul_keys);
      if (!consul_keys.empty()) {
        service_list->insert(consul_keys[0]);
      }
    }
    return true;
  }

  bool getServerList(std::vector<service_router::Server>* server_list, const service_router::Server& server) {
    auto result = kv_->getValuesSync(ConsulNode::getNodesPath(server));
    if (!result) {
      return false;
    }
    auto values = (*result).getValue();
    if (values.empty()) {
      return false;
    }

    for (auto& t : values) {
      std::string vval = proxygen::Base64::decode(t.getValue(), 0);
      folly::dynamic dy_server;
      if (!common::fromJson(&dy_server, vval)) {
        VLOG(4) << "Json parse is fail, vval:" << vval;
        continue;
      }
      service_router::Server target_server;
      if (!target_server.deserialize(dy_server)) {
        VLOG(4) << "Json to object is fail, vval:" << vval;
        continue;
      }
      server_list->push_back(target_server);
    }

    return true;
  }

 private:
  std::shared_ptr<service_framework::consul::KVClient> kv_;
};

// 命令函数注册器
typedef int (*CommandFunction)(ServiceManager*);
using CommandMap = std::unordered_map<std::string, CommandFunction>;
CommandMap g_command_map;
#define RegisterCommandFunction(func)                       \
  namespace {                                               \
  class __Registerer_##func {                               \
   public: /* NOLINT */                                     \
    __Registerer_##func() { g_command_map[#func] = &func; } \
  };                                                        \
  __Registerer_##func g_registerer_##func;                  \
  }

static service_router::Server getServer(const std::string& service_name) {
  service_router::Server server;
  server.setServiceName(service_name);
  return server;
}

int create(ServiceManager* manager) {
  if (FLAGS_service_name.empty()) {
    LOG(INFO) << "Service name need given.";
    return COMMAND_FAIL;
  }

  auto server = getServer(FLAGS_service_name);
  if (!manager->createRouterConfig(server)) {
    LOG(INFO) << "Create service router config fail.";
    return COMMAND_FAIL;
  }
  if (!manager->createConfigs(server)) {
    LOG(INFO) << "Create service configs fail.";
    return COMMAND_FAIL;
  }
  LOG(INFO) << "Create service success.";
  return COMMAND_SUCCESS;
}
RegisterCommandFunction(create);

int listAllService(ServiceManager* manager) {
  std::set<std::string> service_list;
  if (!manager->getServices(&service_list)) {
    return COMMAND_FAIL;
  }
  auto router = service_router::Router::getInstance();
  // 服务名称
  ConsoleTable table;
  table.setHeader({ConsoleTableCell("Name"),         ConsoleTableCell("TotalShards"), ConsoleTableCell("TtlInMs"),
                   ConsoleTableCell("PullInterval"), ConsoleTableCell("LoadBalance"), ConsoleTableCell("Available")});

  for (auto& service_name : service_list) {
    router->waitUntilConfig(service_name, FLAGS_config_timeout);
    router->waitUntilDiscover(service_name, FLAGS_timeout);
    service_router::ServiceRouterConfig config = router->getRouterConfig(service_name);

    std::vector<ConsoleTableCell> cols(
        {ConsoleTableCell(service_name),
         ConsoleTableCell(folly::to<std::string>(config.getTotalShards())),
         ConsoleTableCell(folly::to<std::string>(config.getTtlInMs())),
         ConsoleTableCell(folly::to<std::string>(config.getPullInterval())),
         ConsoleTableCell(service_router::toStringLoadBalanceMethod(config.getLoadBalance()))});
    auto thrift_server_list = router->getServerList(service_name, service_router::ServerProtocol::THRIFT, -1,
                                                    service_router::ShardType::LEADER);
    auto http_server_list = router->getServerList(service_name, service_router::ServerProtocol::HTTP, -1,
                                                  service_router::ShardType::LEADER);
    if (!http_server_list && !thrift_server_list) {
      cols.push_back(ConsoleTableCell(folly::to<std::string>(0), ConsoleTableColor::RED));
    } else {
      size_t size = 0;
      if (http_server_list) {
        size += (*http_server_list)->getServers().size();
      }
      if (thrift_server_list) {
        size += (*thrift_server_list)->getServers().size();
      }
      cols.push_back(ConsoleTableCell(folly::to<std::string>(size), ConsoleTableColor::GREEN));
    }
    table.addRows(cols);
  }
  table.print(std::cout);
  return COMMAND_SUCCESS;
}

int listService(ServiceManager* manager, const service_router::Server& server) {
  std::vector<service_router::Server> server_list;
  if (!manager->getServerList(&server_list, server)) {
    return COMMAND_FAIL;
  }

  auto router = service_router::Router::getInstance();
  router->waitUntilDiscover(server.getServiceName(), FLAGS_timeout);
  auto thrift_server_list = router->getServerList(server.getServiceName(), service_router::ServerProtocol::THRIFT, -1,
                                                  service_router::ShardType::LEADER);
  auto http_server_list = router->getServerList(server.getServiceName(), service_router::ServerProtocol::HTTP, -1,
                                                service_router::ShardType::LEADER);
  std::unordered_map<std::string, service_router::Server> discover_map;
  if (thrift_server_list) {
    for (auto& t : (*thrift_server_list)->getServers()) {
      discover_map[ConsulNode::getNodeName(t)] = t;
    }
  }
  if (http_server_list) {
    for (auto& t : (*http_server_list)->getServers()) {
      discover_map[ConsulNode::getNodeName(t)] = t;
    }
  }
  router->waitUntilConfig(server.getServiceName(), FLAGS_timeout);
  service_router::ServiceRouterConfig config = router->getRouterConfig(server.getServiceName());

  // 服务名称
  ConsoleTable table;
  table.setHeader({ConsoleTableCell("Address"),   ConsoleTableCell("UpdateTime"),
                   // ConsoleTableCell("Status"),
                   ConsoleTableCell("Weight"),    ConsoleTableCell("Heartbeat"),    ConsoleTableCell("Candidate"),
                   ConsoleTableCell("ShardList"), ConsoleTableCell("OtherSettings")});

  std::time_t now = common::currentTimeInMs();
  for (auto& server : server_list) {
    std::string address = ConsulNode::getNodeName(server);

    std::time_t update_time = static_cast<std::time_t>(server.getUpdateTime() / 1000);
    std::stringstream update_time_stream;
    update_time_stream << std::put_time(std::localtime(&update_time), "%Y-%m-%d %H:%M:%S");
    std::string update_time_local = update_time_stream.str();

    std::vector<ConsoleTableCell> cols({ConsoleTableCell(address), ConsoleTableCell(update_time_local),
                                        // ConsoleTableCell(service_router::toStringServerStatus(server.getStatus())),
                                        ConsoleTableCell(folly::to<std::string>(server.getWeight()))});

    if (static_cast<uint64_t>(now) > server.getUpdateTime() + config.getTtlInMs()) {
      cols.push_back(ConsoleTableCell("NO", ConsoleTableColor::RED));
    } else {
      cols.push_back(ConsoleTableCell("YES", ConsoleTableColor::GREEN));
    }

    std::string is_candidate = (discover_map.find(address) != discover_map.end()) ? "YES" : "NO";
    ConsoleTableColor candidate_color =
        (discover_map.find(address) != discover_map.end()) ? ConsoleTableColor::GREEN : ConsoleTableColor::RED;
    cols.push_back(ConsoleTableCell(is_candidate, candidate_color));
    cols.push_back(ConsoleTableCell(""));
    cols.push_back(ConsoleTableCell(""));
    table.addRows(cols);
  }
  table.print(std::cout);
  return COMMAND_SUCCESS;
}

int list(ServiceManager* manager) {
  if (FLAGS_service_name.empty()) {
    return listAllService(manager);
  }

  auto server = getServer(FLAGS_service_name);
  return listService(manager, server);
}
RegisterCommandFunction(list);

static CommandFunction getCommandFunction(const std::string& name) {
  if (g_command_map.count(name)) {
    return g_command_map[name];
  } else {
    LOG(ERROR) << "Available service_manager actions:";
    for (auto& t : g_command_map) {
      LOG(ERROR) << "\t" << t.first;
    }
    LOG(FATAL) << "Unknown action: " << name;
    return nullptr;  // not reachable, just to suppress old compiler warnings.
  }
}

int main(int argc, char* argv[]) {
  // Usage message.
  gflags::SetUsageMessage(
      "command line \n"
      "usage: service_manager <command> <args>\n\n"
      "commands:\n"
      "  create          create a service\n"
      "  list            list all service\n"
      "  clean           clean service");

  folly::init(&argc, &argv);
  FLAGS_logtostderr = true;

  SCOPE_EXIT {
    service_framework::http::stop();
  };

  if (argc == 2) {
    ServiceManager manager;
    return getCommandFunction(folly::to<std::string>(argv[1]))(&manager);
  } else {
    gflags::ShowUsageWithFlagsRestrict(argv[0], "service_router/service_manager");
  }

  return 0;
}
