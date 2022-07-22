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

#pragma once

#include <string>
#include <vector>

#include "folly/io/async/AsyncTimeout.h"

#include "registry.h"
#include "router.h"
#include "service_router_entity.h"

namespace service_router {

DECLARE_int32(router_default_pull_interval);

template <typename Type>
class ServiceInfoPuller {
 public:
  ServiceInfoPuller(const std::string& service_name, folly::EventBase* evb, const Type& pull_type, RouterDb* db)
      : service_name_(service_name), evb_(evb), pull_type_(pull_type), db_(db) {
    timeout_ = folly::AsyncTimeout::make(*evb_, [&]() noexcept {
      pull_type_.pull(service_name_);
      addPullSchedule();
    });
    pull_type_.subscribe(service_name_, this);
    addPullSchedule();
  }

  ~ServiceInfoPuller() {
    VLOG(4) << "Delete pull service info, service name:" << service_name_;
    evb_->runInEventBaseThreadAndWait([this]() { timeout_->cancelTimeout(); });
    VLOG(5) << "Delete service info pull.";
  }

  void wait(bool is_once) {
    if (!is_once) {
      VLOG(4) << "Reset puller semaphore, service_name:" << service_name_;
      semaphore_.reset();
    }
    // 只有在 reset 初始状态下才会主动调用 consul 数据
    if (!semaphore_.ready()) {
      pull_type_.pull(service_name_);
    }
    semaphore_.wait();
    VLOG(4) << "Wait pull complete, service_name:" << service_name_;
  }

  void waitUntil(int milliseconds, bool is_once) {
    if (!is_once) {
      VLOG(4) << "Reset puller semaphore, service_name:" << service_name_;
      semaphore_.reset();
    }

    // 只有在 reset 初始状态下才会主动调用 consul 数据
    if (!semaphore_.ready()) {
      pull_type_.pull(service_name_);
    }
    semaphore_.try_wait_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds));
    VLOG(4) << "Wait until pull complete, service_name:" << service_name_;
  }

  void post() { semaphore_.post(); }

 private:
  std::string service_name_;
  folly::EventBase* evb_;
  Type pull_type_;
  RouterDb* db_;
  std::unique_ptr<folly::AsyncTimeout> timeout_;
  folly::SaturatingSemaphore<true> semaphore_;

  void addPullSchedule() {
    ServiceRouterConfig router_config = db_->getRouterConfig(service_name_);
    // heartbeat 间隔是 router ttl 的 2 / 3
    int pull_interval = router_config.getPullInterval() * 1000;
    if (pull_interval < FLAGS_router_default_pull_interval) {
      pull_interval = FLAGS_router_default_pull_interval;
    }
    VLOG(4) << "Add pull service info, service name:" << service_name_ << " interval:" << pull_interval;
    evb_->runInEventBaseThread(
        [pull_interval, this]() { timeout_->scheduleTimeout(std::chrono::milliseconds(pull_interval)); });
  }
};

class ServiceConfigPull {
 public:
  ServiceConfigPull(RouterDb* db, RegistryInterface* registry);
  ~ServiceConfigPull() = default;
  void pull(const std::string& service_name);
  void subscribe(const std::string& service_name, ServiceInfoPuller<ServiceConfigPull>* puller);

 private:
  RouterDb* db_;
  RegistryInterface* registry_;
};

class ServiceDiscoverPull {
 public:
  ServiceDiscoverPull(RouterDb* db, RegistryInterface* registry);
  ~ServiceDiscoverPull() = default;
  void pull(const std::string& service_name);
  void subscribe(const std::string& service_name, ServiceInfoPuller<ServiceDiscoverPull>* puller);

 private:
  RouterDb* db_;
  RegistryInterface* registry_;
};

class ServerWithHeartbeat {
 public:
  ServerWithHeartbeat(const Server& server, folly::EventBase* evb, Router* router, RouterDb* db);

  const Server& getServer() { return server_; }
  ~ServerWithHeartbeat() {
    evb_->runInEventBaseThreadAndWait([this]() { timeout_->cancelTimeout(); });
  }

  inline ServerStatus getStatus() { return server_.getStatus(); }
  void setStatus(const ServerStatus& status);
  inline void setWeight(int weight) { server_.setWeight(weight); }
  inline void setIdc(const std::string& idc) { server_.setIdc(idc); }
  inline void setDc(const std::string& dc) { server_.setDc(dc); }
  inline void setShardList(const std::vector<uint32_t>& shard_list) { server_.setShardList(shard_list); }
  inline void setAvailableShardList(const std::vector<uint32_t>& shard_list) {
    server_.setAvailableShardList(shard_list);
  }
  inline void setFollowerShardList(const std::vector<uint32_t>& shard_list) {
    server_.setFollowerShardList(shard_list);
  }
  inline void setFollowerAvailableShardList(const std::vector<uint32_t>& shard_list) {
    server_.setFollowerAvailableShardList(shard_list);
  }
  inline void setOtherSettings(const std::unordered_map<std::string, std::string>& other_settings) {
    server_.setOtherSettings(other_settings);
  }
  inline void setOtherSettings(const std::string& key, const std::string& value) {
    auto data = server_.getOtherSettings();
    data[key] = value;
    server_.setOtherSettings(data);
  }

  inline void setIsEdgeNode(bool is_edge_node) { server_.setIsEdgeNode(is_edge_node); }

  inline void setPartitionList(const std::vector<int64_t>& partition_list) { server_.setPartitionList(partition_list); }
  inline void pause() { is_pause_ = true; }
  inline void resume() { is_pause_ = false; }

 private:
  Server server_;
  folly::EventBase* evb_;
  Router* router_;
  RouterDb* db_;
  std::unique_ptr<folly::AsyncTimeout> timeout_;
  void addHeartbeat();
  bool is_pause_{false};
};

}  // namespace service_router
