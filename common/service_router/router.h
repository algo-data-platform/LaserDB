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

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "folly/Random.h"
#include "folly/SharedMutex.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "folly/io/async/ScopedEventBaseThread.h"

#include "common/metrics/metrics.h"
#include "common/util.h"
#include "load_balance.h"
#include "registry.h"
#include "service_router_entity.h"

namespace service_router {

DECLARE_int32(discover_wait_milliseconds);
DECLARE_int32(idle_timeout);
DECLARE_int32(thrift_timeout_retry);
DECLARE_int32(thrift_connection_retry);

constexpr char DEFAULT_DC[] = "default";

class Router;
template <typename Type>
class ServiceInfoPuller;
class ServiceConfigPull;
class ServiceDiscoverPull;
class ServerWithHeartbeat;

class ServerList {
 public:
  explicit ServerList(const std::vector<Server>& server_list);
  ServerList() = default;
  ~ServerList() = default;
  folly::Optional<Server> selectServers(const std::shared_ptr<LoadBalanceInterface>& load) const;
  const std::vector<Server> getServers();
  inline uint64_t getHashCode() { return hash_code_; }

 private:
  std::vector<Server> servers_;
  uint64_t hash_code_{0};
};

class ServiceRegistry {
 public:
  ServiceRegistry() = default;
  ~ServiceRegistry() = default;
  explicit ServiceRegistry(const std::unordered_map<int64_t, std::shared_ptr<ServerList>>& services);
  folly::Optional<std::shared_ptr<ServerList>> selectServers(int64_t key);
  const std::unordered_map<int64_t, std::shared_ptr<ServerList>> getServices();

 private:
  std::unordered_map<int64_t, std::shared_ptr<ServerList>> services_;
};

// RouterDb 职责：
//
//   1. 服务 -> Server 对象的映射关系
//      提供线程安全的查询、更新策略、服务删除
//
//   2. 服务+分区 -> Server
//      提供线程安全的查询、更新策略、服务删除
//
// thread safe
class RouterDb {
 public:
  RouterDb();
  ~RouterDb() = default;
  folly::Optional<std::shared_ptr<ServerList>> selectServers(const std::string& service_name,
                                                             const ServerProtocol& protocol, int64_t route_id,
                                                             const ShardType& type, const std::string& dc = DEFAULT_DC);
  std::unordered_map<int64_t, std::shared_ptr<ServerList>> selectServers(const std::string& service_name,
                                                                         const ServerProtocol& protocol,
                                                                         const std::vector<int64_t>& route_ids,
                                                                         const ShardType& type,
                                                                         const std::string& dc = DEFAULT_DC);
  void updateServers(const std::string& service_name, const std::vector<Server>& list);

  const std::string getConfig(const std::string& name, const std::string& key);
  const ServiceRouterConfig getRouterConfig(const std::string& service_name);
  const ServiceConfig getConfig(const std::string& service_name);
  void updateConfig(const std::string& service_name, const ServiceConfig& config);

 private:
  std::shared_ptr<ServiceRegistry> serviceRegistry_;
  folly::SharedMutexReadPriority update_server_mutex_;
  folly::Synchronized<std::unordered_map<std::string, ServiceConfig>> configs_;
  const std::vector<Server> pickServers(const std::string& service_name, const std::vector<Server>& list);
  int64_t getServiceKey(const std::string& service_name, const ServerProtocol& protocol, int64_t route_id,
                        const ShardType& type, const std::string& dc = DEFAULT_DC);
  std::shared_ptr<metrics::Timers> timers_;
  folly::Synchronized<std::unordered_map<std::string, std::unordered_set<int64_t>>> last_partition_keys_;
};

class RouterThread : public folly::ScopedEventBaseThread {
 public:
  RouterThread() : folly::ScopedEventBaseThread("serviceRouterTimer") {}
  ~RouterThread() = default;
};

// Router 职责：
//  1. 配置管理
//    a. 首次获取配置
//    b. 创建定时 pull 数据定时器
//    c. 设置各种 subscribe 回调
//  2. 服务注册，注销
//    a. 服务自身心跳维护定时器 2/3 * TTL
//    b. 服务注销
//  3. 服务发现
//    a. 首次获取服务列表并进行 cache 映射处理
//    b. 创建定时 pull 数据定时器
//    c. 设置各种 subscribe 回调
//    d. 服务发现自我保护策略实现（consol 、zk 等数据层服务挂掉时）
//    e. 提供非stateful、shared 分区等类型的数据候选节点查询
class Router {
 public:
  static std::shared_ptr<Router> getInstance();
  Router();
  virtual ~Router() { LOG(ERROR) << "Router deleted"; }

  // 用于创建服务注册的 Registry，注册自身服务
  virtual void createServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address);

  // 手动创建Registry
  virtual void setServiceRegistry(const std::string& service_name, std::shared_ptr<RegistryInterface> registry);

  // 用于创建服务发现的 Registry，发现目标服务
  virtual void createTargetServiceRegistry(const std::string& service_name, RegistryType type,
                                           const std::string& address);

  virtual void waitForConfig(const std::string& service_name, bool is_once = true);

  virtual void waitForDiscover(const std::string& service_name, bool is_once = true);

  virtual void waitUntilConfig(const std::string& service_name, int milliseconds, bool is_once = true);

  virtual void waitUntilDiscover(const std::string& service_name, int milliseconds, bool is_once = true);

  virtual folly::Optional<Server> discover(const ClientOption& option);
  virtual folly::Optional<Server> edgeDiscover(const ClientOption& option);
  virtual std::unordered_map<int64_t, folly::Optional<Server>> batchDiscover(const ClientOption& option,
                                                                             const std::vector<int64_t>& route_ids);

  virtual folly::Optional<std::shared_ptr<ServerList>> getServerList(const std::string& service_name,
                                                                     const ServerProtocol& protocol, int64_t route_id,
                                                                     const ShardType& type,
                                                                     const std::string& dc = DEFAULT_DC);
  virtual std::unordered_map<int64_t, std::shared_ptr<ServerList>> getServerList(const std::string& service_name,
                                                                                 const ServerProtocol& protocol,
                                                                                 const std::vector<int64_t>& route_ids,
                                                                                 const ShardType& type,
                                                                                 const std::string& dc = DEFAULT_DC);

  virtual bool registerServerSync(const Server server);

  virtual void registerServer(const Server server);

  virtual void setStatus(const Server& server, const ServerStatus& status);

  virtual folly::Optional<ServerStatus> getStatus(const Server& server);

  virtual void setWeight(const Server& server, int weight);

  virtual void setIdc(const Server& server, const std::string& idc);

  virtual void setDc(const Server& server, const std::string& dc);

  virtual void setShardList(const Server& server, const std::vector<uint32_t>& shard_list);

  virtual void setAvailableShardList(const Server& server, const std::vector<uint32_t>& shard_list);

  virtual void setFollowerShardList(const Server& server, const std::vector<uint32_t>& shard_list);

  virtual void setFollowerAvailableShardList(const Server& server, const std::vector<uint32_t>& shard_list);

  virtual void setOtherSettings(const Server& server,
                                const std::unordered_map<std::string, std::string>& other_settings);

  virtual void setOtherSettings(const Server& server, const std::string& key, const std::string& value);

  virtual void setStatus(const std::string& service_name, const ServerStatus& status);

  virtual folly::Optional<ServerStatus> getStatus(const std::string& service_name);

  virtual void setIsEdgeNode(const Server& server, bool is_edge_node);

  virtual void setPartitionList(const Server& server, const std::vector<int64_t> partition_list);

  virtual bool unregisterServerSync(const Server server);

  virtual void unregisterServer(const Server server);

  virtual void unregisterAll();

  virtual void subscribeConfig(const std::string& service_name, NotifyConfig notify);
  virtual void subscribeConfigItem(const std::string& service_name, const std::string& key, NotifyConfigItem notify);

  virtual void unsubscribeConfig(const std::string& service_name);
  virtual void unsubscribeConfigItem(const std::string& service_name, const std::string& key);

  virtual const ServiceRouterConfig getRouterConfig(const std::string& service_name);

  virtual const std::unordered_map<std::string, std::string> getConfigs(const std::string& service_name);

  virtual void stop();

  // registry name 实际上就是依赖的service name
  void getAllRegistryName(std::vector<std::string>* registry_names);
  std::shared_ptr<RegistryInterface> getRegistry(const std::string& service_name);
  std::shared_ptr<RegistryInterface> getOrCreateRegistry(const std::string& service_name);

 private:
  folly::EventBase* evb_;
  folly::Synchronized<std::unordered_map<std::string, std::shared_ptr<RegistryInterface>>> registries_;
  std::unique_ptr<RouterDb> db_;
  std::unique_ptr<RouterThread> router_thread_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> callback_thread_pool_;
  // 注册的所有的 server
  folly::Synchronized<std::unordered_map<std::string, std::shared_ptr<ServerWithHeartbeat>>> servers_;
  using ServiceConfigInfoPullerPtr = std::shared_ptr<ServiceInfoPuller<ServiceConfigPull>>;
  using ServiceDiscoverPullerPtr = std::shared_ptr<ServiceInfoPuller<ServiceDiscoverPull>>;
  folly::Synchronized<std::unordered_map<std::string, ServiceConfigInfoPullerPtr>> config_pullers_;
  folly::Synchronized<std::unordered_map<std::string, ServiceDiscoverPullerPtr>> discover_pullers_;
  folly::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<LoadBalanceInterface>>> balancers_;

  const std::string getServerKey(const Server& server);
  std::shared_ptr<ServiceInfoPuller<ServiceConfigPull>> getOrCreateConfigPuller(const std::string& service_name);
  std::shared_ptr<ServiceInfoPuller<ServiceDiscoverPull>> getOrCreateDiscoverPuller(const std::string& service_name);
  std::shared_ptr<LoadBalanceInterface> getOrCreateBalance(const ClientOption& option);
  std::shared_ptr<RegistryInterface> createRegistry(const std::string& service_name, RegistryType type,
                                                    const std::string& address);
};

// 用于创建服务注册的 Registry，注册自身服务
void createServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address);

// 用于创建服务发现的 Registry，发现目标服务
void createTargetServiceRegistry(const std::string& service_name, RegistryType type, const std::string& address);

// 通用 服务注册
// 默认会持续阻塞，知道配置获取到才会注册服务
void registerServer(const Server& server, int wait_milliseconds = 0);
bool registerServerSync(const Server& server, int wait_milliseconds = 0);

void unregisterServer(const Server& server);
bool unregisterServerSync(const Server& server);
void unregisterAll();
void stop();

// 订阅配置变更
void subscribeConfig(const std::string& service_name, NotifyConfig notify);
void subscribeConfigItem(const std::string& service_name, const std::string& key, NotifyConfigItem notify);
void unsubscribeConfig(const std::string& service_name);
void unsubscribeConfigItem(const std::string& service_name, const std::string& key);

const std::unordered_map<std::string, std::string> getConfigs(const std::string& service_name,
                                                              int wait_milliseconds = 0);
// 获取服务
// 默认会阻塞直到获取到服务列表
bool serviceClient(ServerAddress* address, const ClientOption& option, int wait_milliseconds = 0);

}  // namespace service_router
