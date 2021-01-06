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
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "folly/Random.h"
#include "folly/Synchronized.h"
#include "folly/ThreadLocal.h"

#include "common/util.h"
#include "service_router_entity.h"

namespace service_router {

class Router;
class LoadBalanceInterface {
 public:
  explicit LoadBalanceInterface(Router* router) : router_(router) {}
  virtual bool select(Server* server, const std::vector<Server>& list) = 0;
  // virtual bool select(Server* server, const folly::Synchronized<std::vector<Server>>& list) = 0;
  virtual ~LoadBalanceInterface() {}

 protected:
  Router* router_;
};

class LoadBalanceIdcFirst : public LoadBalanceInterface {
 public:
  explicit LoadBalanceIdcFirst(Router* router, const std::string& idc) : LoadBalanceInterface(router), idc_(idc) {}
  ~LoadBalanceIdcFirst() = default;
  bool select(Server* server, const std::vector<Server>& list) override;

 private:
  std::string idc_;
};

class LoadBalanceRandom : public LoadBalanceInterface {
 public:
  explicit LoadBalanceRandom(Router* router) : LoadBalanceInterface(router) {}
  ~LoadBalanceRandom() = default;
  bool select(Server* server, const std::vector<Server>& list) override;
};

class LoadBalanceRoundrobin : public LoadBalanceInterface {
 public:
  explicit LoadBalanceRoundrobin(Router* router) : LoadBalanceInterface(router) {}
  ~LoadBalanceRoundrobin() = default;
  bool select(Server* server, const std::vector<Server>& list) override;

 private:
  std::atomic<uint64_t> requests_{0};
};

class LoadBalanceLocalFirst : public LoadBalanceInterface {
 public:
  explicit LoadBalanceLocalFirst(Router* router, const BalanceLocalFirstConfig& config)
      : LoadBalanceInterface(router), config_(config) {
    local_ip_ = common::ipToInt(config_.getLocalIp());
  }
  virtual ~LoadBalanceLocalFirst() = default;
  bool select(Server* server, const std::vector<Server>& list) override;

 protected:
  uint32_t local_ip_{0};
  BalanceLocalFirstConfig config_;
};

class LoadBalanceIpRangeFirst : public LoadBalanceLocalFirst {
 public:
  explicit LoadBalanceIpRangeFirst(Router* router, const BalanceLocalFirstConfig& config)
      : LoadBalanceLocalFirst(router, config) {}
  ~LoadBalanceIpRangeFirst() = default;
  bool select(Server* server, const std::vector<Server>& list) override;
};

class LoadBalanceConfigurableWeight : public LoadBalanceInterface {
 public:
  explicit LoadBalanceConfigurableWeight(Router* router) : LoadBalanceInterface(router) {}
  ~LoadBalanceConfigurableWeight() = default;
  bool select(Server* server, const std::vector<Server>& list) override;

 private:
  struct CurrentStateWrapper {
    int32_t current_index = -1;
    int32_t current_weight = 0;
  };
  folly::ThreadLocal<CurrentStateWrapper> current_state_;

  inline uint32_t gcd(uint32_t a, uint32_t b) {
    if ((a % b) == 0) {
      return b;
    } else {
      return gcd(b, a % b);
    }
  }
};

class LoadBalanceStaticWeight : public LoadBalanceInterface {
 public:
  explicit LoadBalanceStaticWeight(Router* router) : LoadBalanceInterface(router) {}
  ~LoadBalanceStaticWeight() = default;
  bool select(Server* server, const std::vector<Server>& list) override;
};

}  // namespace service_router
