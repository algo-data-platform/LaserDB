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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#include "options.h"

namespace laser {

std::ostream& operator<<(std::ostream& os, const ClientRequestReadMode& value) {
  switch (value) {
    case ClientRequestReadMode::LEADER_READ:
      os << "leader_read";
      break;
    case ClientRequestReadMode::FOLLOWER_READ:
      os << "follower_read";
      break;
    case ClientRequestReadMode::MIXED_READ:
      os << "mixed_read";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeClientRequestReadMode(const ClientRequestReadMode& value) {
  folly::dynamic result = toStringClientRequestReadMode(value);
  return result;
}

bool deserializeClientRequestReadMode(const folly::dynamic& data, ClientRequestReadMode* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToClientRequestReadMode(value);
  if (enum_obj) {
    *result = *enum_obj;
    return true;
  }

  return false;
}

folly::Optional<ClientRequestReadMode> stringToClientRequestReadMode(const std::string& name) {
  if (name == "leader_read") {
    return ClientRequestReadMode::LEADER_READ;
  }
  if (name == "follower_read") {
    return ClientRequestReadMode::FOLLOWER_READ;
  }
  if (name == "mixed_read") {
    return ClientRequestReadMode::MIXED_READ;
  }

  return folly::none;
}

const std::string toStringClientRequestReadMode(const ClientRequestReadMode& value) {
  std::string result;
  switch (value) {
    case ClientRequestReadMode::LEADER_READ:
      result = "leader_read";
      break;
    case ClientRequestReadMode::MIXED_READ:
      result = "mixed_read";
      break;
    case ClientRequestReadMode::FOLLOWER_READ:
      result = "follower_read";
      break;
    default:
      result = "unknow";
  }

  return result;
}

void ClientOption::describe(std::ostream& os) const {
  os << "ClientOption{"
     << "LoadBalance=" << load_balance_ << ", "
     << "LocalFirstConfig=" << local_first_config_ << ", "
     << "MaxConnPerServer=" << max_conn_per_server_ << ", "
     << "ConnectRetry=" << connection_retry_ << ", "
     << "TimeoutRetry=" << timeout_retry_ << ", "
     << "ReceiveTimeoutMs=" << receive_timeout_ms_ << ", "
     << "ThriftCompressionMethod=" << thrift_compression_method_ << ", "
     << "TargetServerAddress=" << target_server_address_ << ", "
     << "ReadMode=" << read_mode_ << "}";
}

std::ostream& operator<<(std::ostream& os, const ClientOption& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ClientOption::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic load_balance = serializeLoadBalanceMethod(load_balance_);
  result.insert("LoadBalance", load_balance);
  folly::dynamic local_first_config = folly::dynamic::object;
  local_first_config = local_first_config_.serialize();
  result.insert("LocalFirstConfig", local_first_config);
  result.insert("MaxConnPerServer", max_conn_per_server_);
  result.insert("ConnectionRetry", connection_retry_);
  result.insert("TimeoutRetry", timeout_retry_);
  result.insert("ReceiveTimeoutMs", receive_timeout_ms_);
  result.insert("ThriftCompressionMethod", thrift_compression_method_);
  folly::dynamic read_mode = serializeClientRequestReadMode(read_mode_);
  result.insert("ReadMode", read_mode);

  return result;
}

bool ClientOption::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* load_balance = data.get_ptr("LoadBalance");
  if (load_balance == nullptr) {
    return false;
  }

  service_router::LoadBalanceMethod itemload_balance;
  if (!deserializeLoadBalanceMethod(*load_balance, &itemload_balance)) {
    return false;
  }
  setLoadBalance(itemload_balance);
  auto* local_first_config = data.get_ptr("LocalFirstConfig");
  if (local_first_config == nullptr) {
    return false;
  }

  service_router::BalanceLocalFirstConfig itemlocal_first_config;
  if (!itemlocal_first_config.deserialize(*local_first_config)) {
    return false;
  }
  setLocalFirstConfig(itemlocal_first_config);
  auto* max_conn_per_server = data.get_ptr("MaxConnPerServer");
  if (max_conn_per_server == nullptr || !max_conn_per_server->isInt()) {
    return false;
  }
  setMaxConnPerServer(max_conn_per_server->asInt());
  auto* receive_timeout = data.get_ptr("ReceiveTimeout");
  if (receive_timeout == nullptr || !receive_timeout->isInt()) {
    return false;
  }
  setReceiveTimeoutMs(receive_timeout->asInt());
  auto* thrift_compression_method = data.get_ptr("ThriftCompressionMethod");
  if (thrift_compression_method == nullptr || !thrift_compression_method->isInt()) {
    return false;
  }
  setThriftCompressionMethod(thrift_compression_method->asInt());
  auto* read_mode = data.get_ptr("ReadMode");
  if (read_mode == nullptr) {
    return false;
  }

  ClientRequestReadMode itemread_mode;
  if (!deserializeClientRequestReadMode(*read_mode, &itemread_mode)) {
    return false;
  }
  setReadMode(itemread_mode);

  return true;
}

}  // namespace laser
