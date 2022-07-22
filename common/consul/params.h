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

#pragma once

#include <string>

#include "proxygen/lib/http/HTTPMessage.h"
#include "proxygen/lib/utils/Base64.h"

namespace service_framework {
namespace consul {

static constexpr char CONSUL_PARAM_KEY_DC[] = "dc";
static constexpr char CONSUL_PARAM_KEY_WAIT[] = "wait";
static constexpr char CONSUL_PARAM_KEY_NEAR[] = "near";
static constexpr char CONSUL_PARAM_KEY_INDEX[] = "index";
static constexpr char CONSUL_PARAM_KEY_FLAGS[] = "flags";
static constexpr char CONSUL_PARAM_KEY_CAS[] = "cas";
static constexpr char CONSUL_PARAM_KEY_ACQUIRE[] = "acquire";
static constexpr char CONSUL_PARAM_KEY_RELEASE[] = "release";
static constexpr char CONSUL_CONSISTENCY_MODE_DEFAULT[] = "default";
static constexpr char CONSUL_CONSISTENCY_MODE_STALE[] = "stale";
static constexpr char CONSUL_CONSISTENCY_MODE_CONSISTENT[] = "consistent";

enum class ConsistencyMode {
  DEFAULT,
  STALE,
  CONSISTENT
};

class QueryParams {
 public:
  QueryParams(const std::string& datacenter, const ConsistencyMode& mode, int64_t wait_time, int64_t index,
              const std::string& near)
      : datacenter_(datacenter), consistency_mode_(mode), index_(index), near_(near) {}

  QueryParams() = default;

  QueryParams(const std::string& datacenter, const ConsistencyMode& mode, int64_t wait_time, int64_t index) {
    QueryParams(datacenter, mode, wait_time, index, "");
  }

  explicit QueryParams(const std::string& datacenter) { QueryParams(datacenter, ConsistencyMode::DEFAULT, -1, -1); }

  QueryParams(const std::string& datacenter, const ConsistencyMode& mode) { QueryParams(datacenter, mode, -1, -1); }

  QueryParams(int64_t wait_time, int64_t index) { QueryParams("", ConsistencyMode::DEFAULT, wait_time, index); }

  QueryParams(const std::string& datacenter, int64_t wait_time, int64_t index) {
    QueryParams(datacenter, ConsistencyMode::DEFAULT, wait_time, index);
  }

  void makeParams(proxygen::HTTPMessage* request) const {
    if (!datacenter_.empty()) {
      request->setQueryParam(CONSUL_PARAM_KEY_DC, proxygen::Base64::urlDecode(datacenter_));
    }
    if (consistency_mode_ != ConsistencyMode::DEFAULT) {
      request->setQueryParam(getConsistency(consistency_mode_), "1");
    }
    if (wait_time_ != -1) {
      request->setQueryParam(CONSUL_PARAM_KEY_WAIT, folly::to<std::string>(wait_time_));
    }
    if (index_ != -1) {
      request->setQueryParam(CONSUL_PARAM_KEY_INDEX, folly::to<std::string>(index_));
    }
    if (!near_.empty()) {
      request->setQueryParam(CONSUL_PARAM_KEY_NEAR, proxygen::Base64::urlDecode(near_));
    }
  }

 private:
  std::string datacenter_;
  // @ref https://www.consul.io/api/index.html#consistency-modes
  ConsistencyMode consistency_mode_;
  // @ref https://www.consul.io/api/index.html#blocking-queries
  int64_t wait_time_{-1};
  int64_t index_{-1};
  std::string near_;

  const std::string getConsistency(const ConsistencyMode& mode) const {
    if (mode == ConsistencyMode::STALE) {
      return std::string(CONSUL_CONSISTENCY_MODE_STALE);
    } else if (mode == ConsistencyMode::CONSISTENT) {
      return std::string(CONSUL_CONSISTENCY_MODE_CONSISTENT);
    } else {
      return std::string(CONSUL_CONSISTENCY_MODE_DEFAULT);
    }
  }
};

class PutParams {
 public:
  PutParams() = default;
  ~PutParams() = default;

  uint64_t getFlags() { return flags_; }

  void setFlags(uint64_t flags) { flags_ = flags; }

  uint64_t getCas() { return cas_; }

  void setCas(uint64_t cas) { cas_ = cas; }

  const std::string& getAcquireSession() const { return acquire_session_; }

  void setAcquireSession(const std::string& acquire_session) { acquire_session_ = acquire_session; }

  const std::string& getReleaseSession() const { return release_session_; }

  void setReleaseSession(const std::string& release_session) { release_session_ = release_session; }

  void makeParams(proxygen::HTTPMessage* request) const {
    if (flags_ != 0) {
      request->setQueryParam(CONSUL_PARAM_KEY_FLAGS, folly::to<std::string>(flags_));
    }
    if (cas_ != 0) {
      request->setQueryParam(CONSUL_PARAM_KEY_CAS, folly::to<std::string>(cas_));
    }
    if (!acquire_session_.empty()) {
      request->setQueryParam(CONSUL_PARAM_KEY_ACQUIRE, proxygen::Base64::urlDecode(acquire_session_));
    }
    if (!release_session_.empty()) {
      request->setQueryParam(CONSUL_PARAM_KEY_RELEASE, proxygen::Base64::urlDecode(release_session_));
    }
  }

 private:
  // @ref https://www.consul.io/docs/guides/leader-election.html
  uint64_t flags_{0};
  uint64_t cas_{0};
  std::string acquire_session_;
  std::string release_session_;
};

}  // namespace consul
}  // namespace service_framework
