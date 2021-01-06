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

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "folly/Function.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "folly/executors/GlobalExecutor.h"

#include "common/metrics/metrics.h"
#include "common/util.h"
#include "common/consul/kv_client.h"
#include "service_router_entity.h"

namespace service_router {

using ConsulValues = service_framework::consul::ConsulResponse<std::vector<service_framework::consul::Value>>;
using NotifyService = folly::Function<void(const std::string& /* service_name */, const std::vector<Server>&)>;
using NotifyConfigItem = folly::Function<void(const std::string& /* service_name */,
                                              const std::string& /* config key */, const std::string&)>;
using NotifyConfig = folly::Function<void(const std::string& /* service_name */, const ServiceConfig&)>;

class RegistryInterface {
 public:
  virtual ~RegistryInterface() = default;

  // 同步方法
  virtual bool registerServerSync(const Server& server) = 0;
  virtual bool unregisterServerSync(const Server& server) = 0;

  // 注册一个 server
  virtual void registerServer(const Server& server) = 0;
  // 注销一个 server
  virtual void unregisterServer(const Server& server) = 0;
  // 设置可用状态
  virtual void available(const Server& server) = 0;
  // 设置不可用状态
  virtual void unavailable(const Server& server) = 0;

  // 发现相关
  virtual void subscribe(const std::string& service_name, NotifyService notify) = 0;
  virtual void unsubscribe(const std::string& service_name) = 0;
  virtual void subscribeConfigItem(const std::string& service_name, const std::string& config_key,
                                   NotifyConfigItem notify) = 0;
  virtual void unsubscribeConfigItem(const std::string& service_name, const std::string& config_key) = 0;
  virtual void subscribeConfig(const std::string& service_name, NotifyConfig notify) = 0;
  virtual void unsubscribeConfig(const std::string& service_name) = 0;

  virtual void discover(const std::string& service_name) = 0;
  virtual void getConfig(const std::string& service_name) = 0;

  // 获取所有订阅的服务名称列表
  virtual void getSubscribedService(std::vector<std::string>* service_names) = 0;

  void enableStatic() { use_static_.store(true); }

  void disableStatic() { use_static_.store(false); }

  void pushStaticNodes(const std::string& service_name, const std::vector<Server>& servers) {
    static_nodes_.withWLock([&](auto& nodes) { nodes[service_name] = servers; });
  }

  const std::vector<Server> getStaticNodes(const std::string& service_name) {
    std::vector<Server> servers;
    static_nodes_.withRLock([&servers, &service_name](auto& nodes) {
      auto iter = nodes.find(service_name);
      if (iter != nodes.end()) {
        servers = iter->second;
      }
    });
    return servers;
  }

 protected:
  std::atomic<bool> use_static_{false};
  folly::Synchronized<std::unordered_map<std::string, std::vector<Server>>> static_nodes_;
};

// registry 是纯异步操作，所以如果请求失败后，会执行重试操作
enum class FallbackType {
  REGISTER,    // 服务注册类型请求重试
  UNREGISTER,  // 服务注销类型请求重试
  DISCOVER,    // 服务发现请求重试
  GET_CONFIG,  // 获取配置请求重试
};

//
// consul 内部存储结构
//
// 通用根路径：/ads-core/services/[service_name]
//
//  /config
//    /router => router 配置 json 串 (ServiceRouterConfig) 序列化
//    /configs
//      /config_item1 => item1_value
//      /config_item2 => item2_value
//      .
//      .
//      .
//      /config_item2 => item2_value
//
//  /nodes
//    /protocol:ip:port => 服务节点信息 json 串 (Service entity 序列化)
//
class ConsulNode {
 public:
  static const std::string getNodeName(const Server& server);
  static const std::string getNodePath(const Server& server);
  static const std::string getNodesPath(const Server& server);
  static const std::string getConfigsPath(const Server& server);
  static const std::string getRouterConfigPath(const Server& server);
  static const std::string getConfigPath(const Server& server);
  static const std::string getServices();
};

class ConsulRegistry : public RegistryInterface {
 public:
  ConsulRegistry() = default;
  explicit ConsulRegistry(folly::EventBase* evb, std::shared_ptr<folly::CPUThreadPoolExecutor> thread_pool,
                          const std::string& address);
  ~ConsulRegistry();
  bool registerServerSync(const Server& server) override;
  bool unregisterServerSync(const Server& server) override;
  // 注册一个 server
  void registerServer(const Server& server) override;
  // 注销一个 server
  void unregisterServer(const Server& server) override;
  // 设置可用状态
  void available(const Server& server) override;
  // 设置不可用状态
  void unavailable(const Server& server) override;

  // 发现相关
  void subscribe(const std::string& service_name, NotifyService notify) override;
  void unsubscribe(const std::string& service_name) override;
  void subscribeConfigItem(const std::string& service_name, const std::string& config_key,
                           NotifyConfigItem notify) override;
  void unsubscribeConfigItem(const std::string& service_name, const std::string& config_key) override;
  void subscribeConfig(const std::string& service_name, NotifyConfig notify) override;
  void unsubscribeConfig(const std::string& service_name) override;

  void discover(const std::string& service_name) override;
  void getConfig(const std::string& service_name) override;

  void getSubscribedService(std::vector<std::string>* services) override;

 private:
  folly::EventBase* evb_;
  folly::Synchronized<std::vector<service_framework::consul::KVClient>> kvs_;
  std::unique_ptr<folly::AsyncTimeout> schedule_;
  std::weak_ptr<folly::CPUThreadPoolExecutor> callback_thread_pool_;
  std::string address_;
  // failback container
  std::shared_ptr<metrics::Counter> ser_reg_count_;
  std::shared_ptr<metrics::Counter> ser_reg_fail_;
  std::shared_ptr<metrics::Counter> ser_get_count_;
  std::shared_ptr<metrics::Counter> ser_get_fail_;
  std::shared_ptr<metrics::Counter> conf_get_count_;
  std::shared_ptr<metrics::Counter> conf_get_fail_;

  class FallBackDetail {
   public:
    FallBackDetail() = default;
    FallBackDetail(const Server& server, const FallbackType& type) : server_(server), type_(type) {}

    const Server& getServer() const { return server_; }
    const FallbackType& getType() const { return type_; }

   private:
    Server server_;
    FallbackType type_;
  };

  folly::Synchronized<std::unordered_map<std::string, FallBackDetail>> fallbacks_;

  // subscribe container
  template <typename Notify, typename Value>
  class NotifyDetail {
   public:
    NotifyDetail(const std::string& service_name, Notify callback)
        : service_name_(service_name), callback_(std::move(callback)) {}

    void notify(const Value& val) { callback_(service_name_, val); }

   private:
    std::string service_name_;
    Notify callback_;
  };

  class NotifyConfigDetail {
   public:
    NotifyConfigDetail(const std::string& service_name, const std::string& key, NotifyConfigItem config_callback)
        : service_name_(service_name), key_(key), config_callback_(std::move(config_callback)) {}

    void notify(const std::string& value) { config_callback_(service_name_, key_, value); }

   private:
    std::string service_name_;
    std::string key_;
    NotifyConfigItem config_callback_;
  };
  using SharedNotifyServiceDetail = std::shared_ptr<NotifyDetail<NotifyService, std::vector<Server>>>;
  using SharedNotifyConfigDetail = std::shared_ptr<NotifyDetail<NotifyConfig, ServiceConfig>>;
  using SharedNotifyConfigItemDetail = std::shared_ptr<NotifyConfigDetail>;
  folly::Synchronized<std::unordered_map<std::string, SharedNotifyServiceDetail>> service_subscribers_;
  folly::Synchronized<std::unordered_map<std::string, SharedNotifyConfigDetail>> config_subscribers_;
  folly::Synchronized<std::unordered_map<std::string, SharedNotifyConfigItemDetail>> config_item_subscribers_;

  // last index
  // 对于 consul 每次请求会返回一个 consul index ，如果发现该值比上次要大说明返回的数据发生了变更
  // 根据该值可以决策是否更新 cache
  folly::Synchronized<std::unordered_map<std::string, uint64_t>> last_indexes_;

  bool checkServerValid(const Server& server);
  void processDiscover(const Server& server, const ConsulValues& values);
  void notifyDiscover(const std::string& service_name, const std::vector<Server>& servers);
  void processConfig(const Server& server, const ConsulValues& values);
  bool checkUpdate(const std::string& key, uint64_t index);
  void fallback();
  service_framework::consul::KVClient getKvClient(bool change = false);
};

class FileRegistry : public RegistryInterface {
 public:
  FileRegistry() = default;
  explicit FileRegistry(uint32_t pull_interval) {
    ServiceRouterConfig router_config;
    router_config.setPullInterval(pull_interval);
    service_config_.setRouter(router_config);
  }
  ~FileRegistry() = default;

  void registerServersFromFile(const std::string& service_name, const std::string& file_path);
  void subscribe(const std::string& service_name, NotifyService notify) override;
  void unsubscribe(const std::string& service_name) override;

  void discover(const std::string& service_name) override;
  void subscribeConfig(const std::string& service_name, NotifyConfig notify) override;
  void unsubscribeConfig(const std::string& service_name) override;
  void getConfig(const std::string& service_name) override;

  // not implemented
  bool registerServerSync(const Server& server) { return true; }
  bool unregisterServerSync(const Server& server) { return true; }
  void registerServer(const Server& server) {}
  void unregisterServer(const Server& server) {}
  void available(const Server& server) {}
  void unavailable(const Server& server) {}
  void getSubscribedService(std::vector<std::string>* services) {}
  void subscribeConfigItem(const std::string& service_name, const std::string& config_key, NotifyConfigItem notify) {}
  void unsubscribeConfigItem(const std::string& service_name, const std::string& config_key) {}

 private:
  class FileServerReader {
   public:
    FileServerReader(const std::string& service_name, const std::string& file_path)
        : service_name_(service_name), file_path_(file_path) {}
    bool getServers(std::vector<service_router::Server>* server_list);

   private:
    bool checkModified();
    bool parseServerFromString(service_router::Server* server, const std::string& line);
    bool splitIpPort(std::string* ip, uint16_t* port, const std::string& str);

   private:
    std::string service_name_;
    std::string file_path_;
    time_t last_modified_time_{0};
  };

  // subscribe container
  template <typename Notify, typename Value>
  class NotifyDetail {
   public:
    NotifyDetail(const std::string& service_name, Notify callback)
        : service_name_(service_name), callback_(std::move(callback)) {}

    void notify(const Value& val) { callback_(service_name_, val); }

   private:
    std::string service_name_;
    Notify callback_;
  };

  class NotifyConfigDetail {
   public:
    NotifyConfigDetail(const std::string& service_name, const std::string& key, NotifyConfigItem config_callback)
        : service_name_(service_name), key_(key), config_callback_(std::move(config_callback)) {}

    void notify(const std::string& value) { config_callback_(service_name_, key_, value); }

   private:
    std::string service_name_;
    std::string key_;
    NotifyConfigItem config_callback_;
  };

 private:
  ServiceConfig service_config_;
  using SharedNotifyServiceDetail = std::shared_ptr<NotifyDetail<NotifyService, std::vector<Server>>>;
  using SharedNotifyConfigDetail = std::shared_ptr<NotifyDetail<NotifyConfig, ServiceConfig>>;
  folly::Synchronized<std::unordered_map<std::string, std::shared_ptr<FileServerReader>>> file_readers_;
  folly::Synchronized<std::unordered_map<std::string, SharedNotifyServiceDetail>> service_subscribers_;
  folly::Synchronized<std::unordered_map<std::string, SharedNotifyConfigDetail>> config_subscribers_;
};

}  // namespace service_router
