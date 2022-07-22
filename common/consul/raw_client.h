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

#include <vector>
#include <string>

#include "folly/dynamic.h"
#include "folly/json.h"
#include "proxygen/lib/http/HTTPMessage.h"
#include "proxygen/lib/utils/URL.h"

#include "common/http/http_client_manager.h"

namespace service_framework {
namespace consul {

static constexpr char CONSUL_RAW_CLIENT_HEADER_X_INDEX[] = "X-Consul-Index";
static constexpr char CONSUL_RAW_CLIENT_HEADER_X_LASTCONTACT[] = "X-Consul-Lastcontact";
static constexpr char CONSUL_RAW_CLIENT_HEADER_X_KNOWNLEADER[] = "X-Consul-Knownleader";
static constexpr char CONSUL_SERVER_SCHEMA[] = "http";

class RawClient {
 public:
  using HttpResponse = std::shared_ptr<service_framework::http::HttpResponse>;
  RawClient(const std::string& host, int port) : host_(host), port_(port) {}

  RawClient(const std::string& host, int port, int connect_timeout, int socket_timeout) : RawClient(host, port) {
    http_option_ = service_framework::http::HttpOption(connect_timeout, socket_timeout);
  }

  RawClient() = default;

  virtual ~RawClient() = default;

 protected:
  const std::string createUrl(const std::string& path) const { return createUrl(CONSUL_SERVER_SCHEMA, path); }
  const std::string createUrl(const std::string& scheme, const std::string& path) const {
    proxygen::URL url(scheme, host_, port_, path);
    return url.getUrl();
  }

  const service_framework::http::HttpOption& getHttpOption() const { return http_option_; }

 private:
  std::string host_;
  int port_;
  service_framework::http::HttpOption http_option_;
};

template <typename ResponseValue>
class ConsulResponseBase {
 public:
  ConsulResponseBase(std::unique_ptr<proxygen::HTTPMessage> message, const ResponseValue& value) : value_(value) {
    proxygen::HTTPHeaders headers = message->getHeaders();

    std::string indexStr = headers.rawGet(CONSUL_RAW_CLIENT_HEADER_X_INDEX);
    if (!indexStr.empty()) {
      auto index_opt = folly::tryTo<uint64_t>(indexStr);
      if (index_opt.hasValue()) {
        consul_index_ = index_opt.value();
      }
    }
    std::string contactStr = headers.rawGet(CONSUL_RAW_CLIENT_HEADER_X_LASTCONTACT);
    if (!contactStr.empty()) {
      auto last_contact_opt = folly::tryTo<uint64_t>(contactStr);
      if (last_contact_opt.hasValue()) {
        consul_last_contact_ = last_contact_opt.value();
      }
    }
    std::string knownleaderStr = headers.rawGet(CONSUL_RAW_CLIENT_HEADER_X_KNOWNLEADER);
    if (!knownleaderStr.empty()) {
      auto known_leader_opt = folly::tryTo<uint64_t>(knownleaderStr);
      if (known_leader_opt.hasValue()) {
        consul_known_leader_ = known_leader_opt.value();
      }
    }
  }

  const ResponseValue& getValue() const { return value_; }

  uint64_t getConsulIndex() const { return consul_index_; }

  uint64_t getConsulLastContact() const { return consul_last_contact_; }

  bool isConsulKnownLeader() const { return consul_known_leader_; }

  virtual ~ConsulResponseBase() = default;

 private:
  uint64_t consul_index_{0};
  uint64_t consul_last_contact_{0};
  bool consul_known_leader_{true};
  ResponseValue value_;
};

template <typename ResponseValue>
class ConsulResponse : public ConsulResponseBase<ResponseValue> {
 public:
  ConsulResponse(std::unique_ptr<proxygen::HTTPMessage> message, const ResponseValue& value)
      : ConsulResponseBase<ResponseValue>(std::move(message), value) {}

  void describe(std::ostream& os) const {
    os << "ConsulResponse{"
       << "value=" << this->getValue() << ", consulIndex=" << this->getConsulIndex()
       << ", consulKnownLeader=" << this->isConsulKnownLeader()
       << ", consulLastContact=" << this->getConsulLastContact() << '}' << std::endl;
  }
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const ConsulResponse<T>& response) {
  response.describe(os);
  return os;
}

template <typename T>
class ConsulResponse<std::vector<T>> : public ConsulResponseBase<std::vector<T>> {
 public:
  ConsulResponse(std::unique_ptr<proxygen::HTTPMessage> message, const std::vector<T>& value)
      : ConsulResponseBase<std::vector<T>>(std::move(message), value) {}

  void describe(std::ostream& os) const {
    os << "ConsulResponse{"
       << "value=[";
    for (auto& t : this->getValue()) {
      os << t << ",";
    }
    os << "], consulIndex=" << this->getConsulIndex() << ", consulKnownLeader=" << this->isConsulKnownLeader()
       << ", consulLastContact=" << this->getConsulLastContact() << '}' << std::endl;
  }
};

}  // namespace consul
}  // namespace service_framework
