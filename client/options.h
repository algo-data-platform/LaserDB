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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#pragma once

#include "common/service_router/service_router_entity.h"

namespace laser {

enum class ClientRequestReadMode {
  LEADER_READ,
  FOLLOWER_READ,
  MIXED_READ
};

folly::dynamic serializeClientRequestReadMode(const ClientRequestReadMode& value);

bool deserializeClientRequestReadMode(const folly::dynamic& data, ClientRequestReadMode* value);

std::ostream& operator<<(std::ostream& os, const ClientRequestReadMode& value);

folly::Optional<ClientRequestReadMode> stringToClientRequestReadMode(const std::string& name);

const std::string toStringClientRequestReadMode(const ClientRequestReadMode& value);

class ClientOption {
 public:
  ClientOption() = default;
  ~ClientOption() = default;

  const service_router::LoadBalanceMethod& getLoadBalance() const { return load_balance_; }

  void setLoadBalance(const service_router::LoadBalanceMethod& load_balance) { load_balance_ = load_balance; }

  const service_router::BalanceLocalFirstConfig& getLocalFirstConfig() const { return local_first_config_; }

  void setLocalFirstConfig(const service_router::BalanceLocalFirstConfig& local_first_config) {
    local_first_config_ = local_first_config;
  }

  uint32_t getMaxConnPerServer() const { return max_conn_per_server_; }

  uint32_t getConnectionRetry() const { return connection_retry_; }

  uint32_t getTimeoutRetry() const { return timeout_retry_; }

  void setMaxConnPerServer(uint32_t max_conn_per_server) { max_conn_per_server_ = max_conn_per_server; }

  uint32_t getReceiveTimeoutMs() const { return receive_timeout_ms_; }

  void setReceiveTimeoutMs(uint32_t receive_timeout_ms) { receive_timeout_ms_ = receive_timeout_ms; }

  void setThriftCompressionMethod(uint16_t thrift_compression_method) {
    thrift_compression_method_ = thrift_compression_method;
  }

  uint16_t getThriftCompressionMethod() const { return thrift_compression_method_; }

  const ClientRequestReadMode& getReadMode() const { return read_mode_; }

  void setReadMode(const ClientRequestReadMode& read_mode) { read_mode_ = read_mode; }

  const service_router::ServerAddress& getTargetServerAddress() const {
    return target_server_address_;
  }
  void setTargetServerAddress(const service_router::ServerAddress& address) {
    target_server_address_ = address;
  }

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  service_router::LoadBalanceMethod load_balance_;
  service_router::BalanceLocalFirstConfig local_first_config_;
  uint32_t max_conn_per_server_{0};
  uint32_t connection_retry_{0};
  uint32_t timeout_retry_{0};
  uint32_t receive_timeout_ms_{0};
  uint16_t thrift_compression_method_{3};
  ClientRequestReadMode read_mode_{ClientRequestReadMode::MIXED_READ};
  service_router::ServerAddress target_server_address_;
};

std::ostream& operator<<(std::ostream& os, const ClientOption& value);

}  // namespace laser
