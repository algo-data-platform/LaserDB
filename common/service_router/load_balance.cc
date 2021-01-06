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

#include "load_balance.h"

namespace service_router {

bool LoadBalanceRandom::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }
  uint32_t size = list.size();
  uint32_t index = folly::Random::secureRand32(0, size);
  *server = list.at(index);
  return true;
}

bool LoadBalanceRoundrobin::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }
  uint64_t requests = requests_.fetch_add(1);
  uint32_t size = list.size();
  *server = list.at(requests % size);
  return true;
}

bool LoadBalanceIdcFirst::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }

  if (idc_ != "") {
    std::vector<Server> refers;
    for (auto& refer : list) {
      if (refer.getIdc() == idc_) {
        refers.push_back(refer);
      }
    }
    // 多个候选机器随机选择一个
    if (!refers.empty()) {
      uint32_t size = refers.size();
      uint32_t index = (size > 1) ? folly::Random::secureRand32(0, size) : 0;
      *server = refers.at(index);
      return true;
    }
  }

  // 没有合适的同机房机器则随机选择一个
  uint32_t size = list.size();
  uint32_t index = (size > 1) ? folly::Random::secureRand32(0, size) : 0;
  *server = list.at(index);
  return true;
}

bool LoadBalanceLocalFirst::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }
  std::vector<Server> refers;
  for (auto& refer : list) {
    uint32_t ip = refer.getHostLong();
    if (ip == 0) {
      continue;
    }

    // 如果本地机器直接返回
    if (ip == local_ip_) {
      *server = refer;
      return true;
    }

    // 同机房机器
    int diff = ip - local_ip_;
    if (std::abs(diff) <= config_.getDiffRange()) {
      refers.push_back(refer);
    }
  }

  // 多个候选机器随机选择一个
  if (!refers.empty()) {
    uint32_t size = refers.size();
    uint32_t index = (size > 1) ? folly::Random::secureRand32(0, size) : 0;
    *server = refers.at(index);
    return true;
  }

  // 没有合适的同机房机器则随机选择一个
  uint32_t size = list.size();
  uint32_t index = (size > 1) ? folly::Random::secureRand32(0, size) : 0;
  *server = list.at(index);
  return true;
}

bool LoadBalanceIpRangeFirst::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }

  std::vector<Server> candidates;
  for (auto& server : list) {
    uint32_t ip = server.getHostLong();
    if (ip == 0) {
      continue;
    }

    // 同机房机器
    int diff = ip - local_ip_;
    if (std::abs(diff) <= config_.getDiffRange()) {
      candidates.push_back(server);
    }
  }

  // 多个候选机器随机选择一个
  if (!candidates.empty()) {
    uint32_t size = candidates.size();
    uint32_t index = folly::Random::secureRand32(0, size);
    *server = candidates.at(index);
    return true;
  }

  // 没有合适的同机房机器则随机选择一个
  uint32_t size = list.size();
  uint32_t index = folly::Random::secureRand32(0, size);
  *server = list.at(index);
  return true;
}

// @refer https://blog.csdn.net/lululove19870526/article/details/53101490
bool LoadBalanceConfigurableWeight::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }
  int32_t gcd_weight = 0;
  int32_t max_weight = 0;
  for (int index = 0, len = list.size(); index < len - 1; index++) {
    max_weight = std::max(max_weight, list.at(index).getWeight());
    if (index == 0) {
      gcd_weight = gcd(list.at(index).getWeight(), list.at(index + 1).getWeight());
    } else {
      gcd_weight = gcd(gcd_weight, list.at(index).getWeight());
    }
  }
  max_weight = std::max(max_weight, list.at(list.size() - 1).getWeight());

  while (true) {
    CurrentStateWrapper* state = current_state_.get();
    state->current_index = (state->current_index + 1) % list.size();
    if (state->current_index == 0) {
      state->current_weight = state->current_weight - gcd_weight;
      if (state->current_weight <= 0) {
        state->current_weight = max_weight;
      }
      if (state->current_weight == 0) {
        return false;
      }
    }
    if (list.at(state->current_index).getWeight() >= state->current_weight) {
      *server = list.at(state->current_index);
      return true;
    }
  }
}

bool LoadBalanceStaticWeight::select(Server* server, const std::vector<Server>& list) {
  if (list.empty()) {
    return false;
  }
  int weight_sum = 0;
  for (const auto& refer : list) {
    weight_sum += refer.getWeight();
  }
  // if all list have 0 weight, then let's randomly pick one, which is
  // basically same with the case where every server has the same weight
  if (0 == weight_sum) {
    uint32_t size = list.size();
    uint32_t index = folly::Random::secureRand32(0, size);
    *server = list.at(index);
    return true;
  }

  thread_local unsigned seed = 0;
  int index = rand_r(&seed) % weight_sum;
  for (auto& refer : list) {
    if (index < refer.getWeight()) {
      *server = refer;
      return true;
    }
    index -= refer.getWeight();
  }
  return false;
}

}  // namespace service_router
