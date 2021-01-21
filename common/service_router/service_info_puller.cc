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

#include "router.h"
#include "service_info_puller.h"

namespace service_router {

DEFINE_int32(router_default_heartbeat_interval, 1000, "Service router default heartbeat interval call");
DEFINE_int32(router_default_pull_interval, 1000, "Service router default pull interval call");

ServerWithHeartbeat::ServerWithHeartbeat(const Server& server, folly::EventBase* evb, Router* router, RouterDb* db)
    : server_(server), evb_(evb), router_(router), db_(db) {
  timeout_ = folly::AsyncTimeout::make(*evb_, [&]() noexcept {
    if (!is_pause_) {
      std::time_t now = common::currentTimeInMs();
      server_.setUpdateTime(static_cast<uint64_t>(now));
    }
    router_->registerServer(server_);
    addHeartbeat();
    VLOG(5) << "Callback heartbeat server:" << server_;
  });
  addHeartbeat();
}

void ServerWithHeartbeat::addHeartbeat() {
  ServiceRouterConfig router_config = db_->getRouterConfig(server_.getServiceName());
  // heartbeat 间隔是 router ttl 的 2 / 3
  int heartbeat_interval = static_cast<int>(2 * router_config.getTtlInMs() / 3);
  if (heartbeat_interval < FLAGS_router_default_heartbeat_interval) {
    heartbeat_interval = FLAGS_router_default_heartbeat_interval;
  }
  VLOG(5) << "Add heartbeat server:" << server_ << " interval:" << heartbeat_interval;
  evb_->runInEventBaseThread([heartbeat_interval, this]() {
    timeout_->scheduleTimeout(std::chrono::milliseconds(heartbeat_interval));
  });
}

void ServerWithHeartbeat::setStatus(const ServerStatus& status) {
  server_.setStatus(status);
  if (status == ServerStatus::AVAILABLE) {
    resume();
  } else {
    pause();
  }
}

ServiceConfigPull::ServiceConfigPull(RouterDb* db, RegistryInterface* registry) : db_(db), registry_(registry) {}

void ServiceConfigPull::pull(const std::string& service_name) { registry_->getConfig(service_name); }

void ServiceConfigPull::subscribe(const std::string& service_name, ServiceInfoPuller<ServiceConfigPull>* puller) {
  registry_->subscribeConfig(service_name,
                             [puller, this](const std::string& name, const ServiceConfig& service_config) {
    db_->updateConfig(name, service_config);
    VLOG(5) << "Service config update, value:" << service_config;
    puller->post();
  });
}

ServiceDiscoverPull::ServiceDiscoverPull(RouterDb* db, RegistryInterface* registry) : db_(db), registry_(registry) {}

void ServiceDiscoverPull::pull(const std::string& service_name) { registry_->discover(service_name); }

void ServiceDiscoverPull::subscribe(const std::string& service_name, ServiceInfoPuller<ServiceDiscoverPull>* puller) {
  VLOG(2) << "subscribe service discover.";
  registry_->subscribe(service_name, [puller, this](const std::string& name, const std::vector<Server>& list) {
    db_->updateServers(name, list);
    VLOG(2) << "Service list discover update, list size:" << list.size();
    puller->post();
  });
}

}  // namespace service_router
