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

#include "service_router_entity.h"

namespace service_router {


std::ostream& operator<<(std::ostream& os, const ThriftTransport& value) {
  switch(value) {
    case ThriftTransport::HEADER:
      os << "header";
      break;
    case ThriftTransport::RSOCKET:
      os << "rsocket";
      break;
    case ThriftTransport::HTTP2:
      os << "http2";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeThriftTransport(const ThriftTransport& value) {
  folly::dynamic result = toStringThriftTransport(value);
  return result;
}

bool deserializeThriftTransport(const folly::dynamic& data, ThriftTransport* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToThriftTransport(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<ThriftTransport> stringToThriftTransport(const std::string& name) {
  if (name == "header") {
    return ThriftTransport::HEADER;
  }
  if (name == "rsocket") {
    return ThriftTransport::RSOCKET;
  }
  if (name == "http2") {
    return ThriftTransport::HTTP2;
  }
     
  return folly::none;
}

const std::string toStringThriftTransport(const ThriftTransport& value) {
  std::string result;
  switch(value) {
    case ThriftTransport::HEADER:
      result = "header";
      break;
    case ThriftTransport::RSOCKET:
      result = "rsocket";
      break;
    case ThriftTransport::HTTP2:
      result = "http2";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const RegistryType& value) {
  switch(value) {
    case RegistryType::CONSUL:
      os << "consul";
      break;
    case RegistryType::AGENT:
      os << "agent";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeRegistryType(const RegistryType& value) {
  folly::dynamic result = toStringRegistryType(value);
  return result;
}

bool deserializeRegistryType(const folly::dynamic& data, RegistryType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToRegistryType(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<RegistryType> stringToRegistryType(const std::string& name) {
  if (name == "consul") {
    return RegistryType::CONSUL;
  }
  if (name == "agent") {
    return RegistryType::AGENT;
  }
     
  return folly::none;
}

const std::string toStringRegistryType(const RegistryType& value) {
  std::string result;
  switch(value) {
    case RegistryType::CONSUL:
      result = "consul";
      break;
    case RegistryType::AGENT:
      result = "agent";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const ServerStatus& value) {
  switch(value) {
    case ServerStatus::AVAILABLE:
      os << "available";
      break;
    case ServerStatus::UNAVAILABLE:
      os << "unavailable";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeServerStatus(const ServerStatus& value) {
  folly::dynamic result = toStringServerStatus(value);
  return result;
}

bool deserializeServerStatus(const folly::dynamic& data, ServerStatus* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToServerStatus(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<ServerStatus> stringToServerStatus(const std::string& name) {
  if (name == "available") {
    return ServerStatus::AVAILABLE;
  }
  if (name == "unavailable") {
    return ServerStatus::UNAVAILABLE;
  }
     
  return folly::none;
}

const std::string toStringServerStatus(const ServerStatus& value) {
  std::string result;
  switch(value) {
    case ServerStatus::AVAILABLE:
      result = "available";
      break;
    case ServerStatus::UNAVAILABLE:
      result = "unavailable";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const ServerProtocol& value) {
  switch(value) {
    case ServerProtocol::HTTP:
      os << "http";
      break;
    case ServerProtocol::THRIFT:
      os << "thrift";
      break;
    case ServerProtocol::REDIS:
      os << "redis";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeServerProtocol(const ServerProtocol& value) {
  folly::dynamic result = toStringServerProtocol(value);
  return result;
}

bool deserializeServerProtocol(const folly::dynamic& data, ServerProtocol* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToServerProtocol(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<ServerProtocol> stringToServerProtocol(const std::string& name) {
  if (name == "http") {
    return ServerProtocol::HTTP;
  }
  if (name == "thrift") {
    return ServerProtocol::THRIFT;
  }
  if (name == "redis") {
    return ServerProtocol::REDIS;
  }
     
  return folly::none;
}

const std::string toStringServerProtocol(const ServerProtocol& value) {
  std::string result;
  switch(value) {
    case ServerProtocol::HTTP:
      result = "http";
      break;
    case ServerProtocol::THRIFT:
      result = "thrift";
      break;
    case ServerProtocol::REDIS:
      result = "redis";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const LoadBalanceMethod& value) {
  switch(value) {
    case LoadBalanceMethod::RANDOM:
      os << "random";
      break;
    case LoadBalanceMethod::ROUNDROBIN:
      os << "roundrobin";
      break;
    case LoadBalanceMethod::LOCALFIRST:
      os << "localfirst";
      break;
    case LoadBalanceMethod::CONSISTENT:
      os << "consistent";
      break;
    case LoadBalanceMethod::CONFIGURABLE_WEIGHT:
      os << "configurable_weight";
      break;
    case LoadBalanceMethod::ACTIVE_WEIGHT:
      os << "active_weight";
      break;
    case LoadBalanceMethod::USER_DEFINED:
      os << "user_defined";
      break;
    case LoadBalanceMethod::IPRANGEFIRST:
      os << "iprangefirst";
      break;
    case LoadBalanceMethod::STATIC_WEIGHT:
      os << "static_weight";
      break;
    case LoadBalanceMethod::IDCFIRST:
      os << "idcfirst";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeLoadBalanceMethod(const LoadBalanceMethod& value) {
  folly::dynamic result = toStringLoadBalanceMethod(value);
  return result;
}

bool deserializeLoadBalanceMethod(const folly::dynamic& data, LoadBalanceMethod* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToLoadBalanceMethod(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<LoadBalanceMethod> stringToLoadBalanceMethod(const std::string& name) {
  if (name == "random") {
    return LoadBalanceMethod::RANDOM;
  }
  if (name == "roundrobin") {
    return LoadBalanceMethod::ROUNDROBIN;
  }
  if (name == "localfirst") {
    return LoadBalanceMethod::LOCALFIRST;
  }
  if (name == "consistent") {
    return LoadBalanceMethod::CONSISTENT;
  }
  if (name == "configurable_weight") {
    return LoadBalanceMethod::CONFIGURABLE_WEIGHT;
  }
  if (name == "active_weight") {
    return LoadBalanceMethod::ACTIVE_WEIGHT;
  }
  if (name == "user_defined") {
    return LoadBalanceMethod::USER_DEFINED;
  }
  if (name == "iprangefirst") {
    return LoadBalanceMethod::IPRANGEFIRST;
  }
  if (name == "static_weight") {
    return LoadBalanceMethod::STATIC_WEIGHT;
  }
  if (name == "idcfirst") {
    return LoadBalanceMethod::IDCFIRST;
  }
     
  return folly::none;
}

const std::string toStringLoadBalanceMethod(const LoadBalanceMethod& value) {
  std::string result;
  switch(value) {
    case LoadBalanceMethod::RANDOM:
      result = "random";
      break;
    case LoadBalanceMethod::ROUNDROBIN:
      result = "roundrobin";
      break;
    case LoadBalanceMethod::LOCALFIRST:
      result = "localfirst";
      break;
    case LoadBalanceMethod::CONSISTENT:
      result = "consistent";
      break;
    case LoadBalanceMethod::CONFIGURABLE_WEIGHT:
      result = "configurable_weight";
      break;
    case LoadBalanceMethod::ACTIVE_WEIGHT:
      result = "active_weight";
      break;
    case LoadBalanceMethod::USER_DEFINED:
      result = "user_defined";
      break;
    case LoadBalanceMethod::IPRANGEFIRST:
      result = "iprangefirst";
      break;
    case LoadBalanceMethod::STATIC_WEIGHT:
      result = "static_weight";
      break;
    case LoadBalanceMethod::IDCFIRST:
      result = "idcfirst";
      break;
    default:
      result = "unknow";
  }

  return result;
}


void ServerAddress::describe(std::ostream& os) const {
  os << "ServerAddress{"
     << "Host='" << host_ << "'"
     << ", " << "Port=" << port_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ServerAddress& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ServerAddress::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Host", host_);
  result.insert("Port", port_);

  return result;
}

bool ServerAddress::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* host = data.get_ptr("Host");
  if (host == nullptr || !host->isString()) {
    return false;
  }
  setHost(host->asString());
  auto* port = data.get_ptr("Port");
  if (port == nullptr || !port->isInt()) {
    return false;
  }
  setPort(port->asInt());

  return true;
}

void BalanceLocalFirstConfig::describe(std::ostream& os) const {
  os << "BalanceLocalFirstConfig{"
     << "LocalIp='" << local_ip_ << "'"
     << ", " << "DiffRange=" << diff_range_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const BalanceLocalFirstConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic BalanceLocalFirstConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("LocalIp", local_ip_);
  result.insert("DiffRange", diff_range_);

  return result;
}

bool BalanceLocalFirstConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* local_ip = data.get_ptr("LocalIp");
  if (local_ip == nullptr || !local_ip->isString()) {
    return false;
  }
  setLocalIp(local_ip->asString());
  auto* diff_range = data.get_ptr("DiffRange");
  if (diff_range == nullptr || !diff_range->isInt()) {
    return false;
  }
  setDiffRange(diff_range->asInt());

  return true;
}

std::ostream& operator<<(std::ostream& os, const ShardType& value) {
  os << static_cast<int>(value);
  return os;
}

folly::dynamic serializeShardType(const ShardType& value) {
  folly::dynamic result = static_cast<int>(value);
  return result;
}

bool deserializeShardType(const folly::dynamic& data, ShardType* result) {
  if (!data.isInt()) {
    return false;
  }

  int value = data.asInt();
  switch(value) {
    case 0:
      *result = ShardType::ALL;
      break;
    case 1:
      *result = ShardType::LEADER;
      break;
    case 2:
      *result = ShardType::FOLLOWER;
      break;
    default:
      return false;
  }

  return true;
}

void ClientOption::describe(std::ostream& os) const {
  os << "ClientOption{"
     << "ServiceName='" << service_name_ << "'"
     << ", " << "ShardId=" << shard_id_
     << ", " << "PartitionHash=" << partition_hash_
     << ", " << "RouteToEdgeNode=" << route_to_edge_node_
     << ", " << "ShardType=" << shard_type_
     << ", " << "Protocol=" << protocol_
     << ", " << "LoadBalance=" << load_balance_
     << ", " << "TargetServerAddress=" << target_server_address_
     << ", " << "TimeoutMs=" << timeout_ms_
     << ", " << "UserBalance='" << user_balance_ << "'"
     << ", " << "LocalFirstConfig=" << local_first_config_
     << ", " << "MaxConnPerServer=" << max_conn_per_server_
     << ", " << "ThriftCompressionMethod=" << thrift_compression_method_
     << ", " << "Idc='" << idc_ << "'"
     << ", " << "Dc='" << dc_ << "'"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ClientOption& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ClientOption::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("ServiceName", service_name_);
  result.insert("ShardId", shard_id_);
  result.insert("PartitionHash", partition_hash_);
  result.insert("RouteToEdgeNode", route_to_edge_node_);
  folly::dynamic shard_type = serializeShardType(shard_type_);
  result.insert("ShardType", shard_type);
  folly::dynamic protocol = serializeServerProtocol(protocol_);
  result.insert("Protocol", protocol);
  folly::dynamic load_balance = serializeLoadBalanceMethod(load_balance_);
  result.insert("LoadBalance", load_balance);
  folly::dynamic target_server_address = folly::dynamic::object;
  target_server_address = target_server_address_.serialize();
  result.insert("TargetServerAddress", target_server_address);
  result.insert("TimeoutMs", timeout_ms_);
  result.insert("UserBalance", user_balance_);
  folly::dynamic local_first_config = folly::dynamic::object;
  local_first_config = local_first_config_.serialize();
  result.insert("LocalFirstConfig", local_first_config);
  result.insert("MaxConnPerServer", max_conn_per_server_);
  result.insert("ThriftCompressionMethod", thrift_compression_method_);
  result.insert("Idc", idc_);
  result.insert("Dc", dc_);

  return result;
}

bool ClientOption::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* service_name = data.get_ptr("ServiceName");
  if (service_name == nullptr || !service_name->isString()) {
    return false;
  }
  setServiceName(service_name->asString());
  auto* shard_id = data.get_ptr("ShardId");
  if (shard_id == nullptr || !shard_id->isInt()) {
    return false;
  }
  setShardId(shard_id->asInt());
  auto* partition_hash = data.get_ptr("PartitionHash");
  if (partition_hash == nullptr || !partition_hash->isInt()) {
    return false;
  }
  setPartitionHash(partition_hash->asInt());
  auto* route_to_edge_node = data.get_ptr("RouteToEdgeNode");
  if (route_to_edge_node == nullptr || !route_to_edge_node->isBool()) {
    return false;
  }
  setRouteToEdgeNode(route_to_edge_node->asBool());
  auto* shard_type = data.get_ptr("ShardType");
  if (shard_type == nullptr) {
    return false;
  }

  ShardType itemshard_type;
  if (!deserializeShardType(*shard_type, &itemshard_type)) {
    return false;
  }
  setShardType(itemshard_type);
  auto* protocol = data.get_ptr("Protocol");
  if (protocol == nullptr) {
    return false;
  }

  ServerProtocol itemprotocol;
  if (!deserializeServerProtocol(*protocol, &itemprotocol)) {
    return false;
  }
  setProtocol(itemprotocol);
  auto* load_balance = data.get_ptr("LoadBalance");
  if (load_balance == nullptr) {
    return false;
  }

  LoadBalanceMethod itemload_balance;
  if (!deserializeLoadBalanceMethod(*load_balance, &itemload_balance)) {
    return false;
  }
  setLoadBalance(itemload_balance);
  auto* target_server_address = data.get_ptr("TargetServerAddress");
  if (target_server_address == nullptr) {
    return false;
  }

  ServerAddress itemtarget_server_address;
  if (!itemtarget_server_address.deserialize(*target_server_address)) {
    return false;
  }
  setTargetServerAddress(itemtarget_server_address);
  auto* timeout_ms = data.get_ptr("TimeoutMs");
  if (timeout_ms == nullptr || !timeout_ms->isInt()) {
    return false;
  }
  setTimeoutMs(timeout_ms->asInt());
  auto* user_balance = data.get_ptr("UserBalance");
  if (user_balance == nullptr || !user_balance->isString()) {
    return false;
  }
  setUserBalance(user_balance->asString());
  auto* local_first_config = data.get_ptr("LocalFirstConfig");
  if (local_first_config == nullptr) {
    return false;
  }

  BalanceLocalFirstConfig itemlocal_first_config;
  if (!itemlocal_first_config.deserialize(*local_first_config)) {
    return false;
  }
  setLocalFirstConfig(itemlocal_first_config);
  auto* max_conn_per_server = data.get_ptr("MaxConnPerServer");
  if (max_conn_per_server == nullptr || !max_conn_per_server->isInt()) {
    return false;
  }
  setMaxConnPerServer(max_conn_per_server->asInt());
  auto* thrift_compression_method = data.get_ptr("ThriftCompressionMethod");
  if (thrift_compression_method == nullptr || !thrift_compression_method->isInt()) {
    return false;
  }
  setThriftCompressionMethod(thrift_compression_method->asInt());
  auto* idc = data.get_ptr("Idc");
  if (idc == nullptr || !idc->isString()) {
    return false;
  }
  setIdc(idc->asString());
  auto* dc = data.get_ptr("Dc");
  if (dc == nullptr || !dc->isString()) {
    return false;
  }
  setDc(dc->asString());

  return true;
}

void ServerOption::describe(std::ostream& os) const {
  os << "ServerOption{"
     << "ServiceName='" << service_name_ << "'"
     << ", " << "ServerAddress=" << server_address_
     << ", " << "ReusePort=" << reuse_port_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ServerOption& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ServerOption::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("ServiceName", service_name_);
  folly::dynamic server_address = folly::dynamic::object;
  server_address = server_address_.serialize();
  result.insert("ServerAddress", server_address);
  result.insert("ReusePort", reuse_port_);

  return result;
}

bool ServerOption::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* service_name = data.get_ptr("ServiceName");
  if (service_name == nullptr || !service_name->isString()) {
    return false;
  }
  setServiceName(service_name->asString());
  auto* server_address = data.get_ptr("ServerAddress");
  if (server_address == nullptr) {
    return false;
  }

  ServerAddress itemserver_address;
  if (!itemserver_address.deserialize(*server_address)) {
    return false;
  }
  setServerAddress(itemserver_address);
  auto* reuse_port = data.get_ptr("ReusePort");
  if (reuse_port == nullptr || !reuse_port->isBool()) {
    return false;
  }
  setReusePort(reuse_port->asBool());

  return true;
}

void Server::describe(std::ostream& os) const {
  os << "Server{"
     << "Host='" << host_ << "'"
     << ", " << "Port=" << port_
     << ", " << "ServiceName='" << service_name_ << "'"
     << ", " << "Protocol=" << protocol_
     << ", " << "Status=" << status_
     << ", " << "Idc='" << idc_ << "'"
     << ", " << "Dc='" << dc_ << "'"
     << ", " << "UpdateTime=" << update_time_
     << ", " << "Weight=" << weight_
     << ", " << "HostLong=" << host_long_
     << ", " << "ShardList=[";
  for (auto& t : shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "AvailableShardList=[";
  for (auto& t : available_shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "FollowerShardList=[";
  for (auto& t : follower_shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "FollowerAvailableShardList=[";
  for (auto& t : follower_available_shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "OtherSettings={";
  for (auto& t : other_settings_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "IsEdgeNode=" << is_edge_node_
     << ", " << "PartitionList=[";
  for (auto& t : partition_list_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Server& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Server::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("Host", host_);
  result.insert("Port", port_);
  result.insert("ServiceName", service_name_);
  folly::dynamic protocol = serializeServerProtocol(protocol_);
  result.insert("Protocol", protocol);
  folly::dynamic status = serializeServerStatus(status_);
  result.insert("Status", status);
  result.insert("Idc", idc_);
  result.insert("Dc", dc_);
  result.insert("UpdateTime", update_time_);
  result.insert("Weight", weight_);
  result.insert("HostLong", host_long_);
  folly::dynamic shard_list = folly::dynamic::array;
  for (auto& t : shard_list_) {
    shard_list.push_back(t);
  }
  result.insert("ShardList", shard_list);
  folly::dynamic available_shard_list = folly::dynamic::array;
  for (auto& t : available_shard_list_) {
    available_shard_list.push_back(t);
  }
  result.insert("AvailableShardList", available_shard_list);
  folly::dynamic follower_shard_list = folly::dynamic::array;
  for (auto& t : follower_shard_list_) {
    follower_shard_list.push_back(t);
  }
  result.insert("FollowerShardList", follower_shard_list);
  folly::dynamic follower_available_shard_list = folly::dynamic::array;
  for (auto& t : follower_available_shard_list_) {
    follower_available_shard_list.push_back(t);
  }
  result.insert("FollowerAvailableShardList", follower_available_shard_list);
  folly::dynamic other_settings = folly::dynamic::object;
  for (auto& t : other_settings_) {
    other_settings.insert(t.first, t.second);
  }
  result.insert("OtherSettings", other_settings);
  result.insert("IsEdgeNode", is_edge_node_);
  folly::dynamic partition_list = folly::dynamic::array;
  for (auto& t : partition_list_) {
    partition_list.push_back(t);
  }
  result.insert("PartitionList", partition_list);

  return result;
}

bool Server::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* host = data.get_ptr("Host");
  if (host == nullptr || !host->isString()) {
    return false;
  }
  setHost(host->asString());
  auto* port = data.get_ptr("Port");
  if (port == nullptr || !port->isInt()) {
    return false;
  }
  setPort(port->asInt());
  auto* service_name = data.get_ptr("ServiceName");
  if (service_name == nullptr || !service_name->isString()) {
    return false;
  }
  setServiceName(service_name->asString());
  auto* protocol = data.get_ptr("Protocol");
  if (protocol == nullptr) {
    return false;
  }

  ServerProtocol itemprotocol;
  if (!deserializeServerProtocol(*protocol, &itemprotocol)) {
    return false;
  }
  setProtocol(itemprotocol);
  auto* status = data.get_ptr("Status");
  if (status == nullptr) {
    return false;
  }

  ServerStatus itemstatus;
  if (!deserializeServerStatus(*status, &itemstatus)) {
    return false;
  }
  setStatus(itemstatus);
  auto* idc = data.get_ptr("Idc");
  if (idc && idc->isString()) {
    setIdc(idc->asString());
  }
  auto* dc = data.get_ptr("Dc");
  if (dc && dc->isString()) {
    setDc(dc->asString());
  }
  auto* update_time = data.get_ptr("UpdateTime");
  if (update_time == nullptr || !update_time->isInt()) {
    return false;
  }
  setUpdateTime(update_time->asInt());
  auto* weight = data.get_ptr("Weight");
  if (weight == nullptr || !weight->isInt()) {
    return false;
  }
  setWeight(weight->asInt());
  auto* host_long = data.get_ptr("HostLong");
  if (host_long && host_long->isInt()) {
    setHostLong(host_long->asInt());
  }
  auto* shard_list = data.get_ptr("ShardList");
  if (shard_list == nullptr || !shard_list->isArray()) {
    return false;
  }

  std::vector<uint32_t> vec_shard_list;
  for (size_t i = 0; i < shard_list->size(); i++) {
    if (!shard_list->at(i).isInt()) {
      return false;
    }
    vec_shard_list.push_back(shard_list->at(i).asInt());
  }
  setShardList(vec_shard_list);
  auto* available_shard_list = data.get_ptr("AvailableShardList");
  if (available_shard_list && available_shard_list->isArray()) {
    std::vector<uint32_t> vec_available_shard_list;
    for (size_t i = 0; i < available_shard_list->size(); i++) {
    if (!available_shard_list->at(i).isInt()) {
      return false;
    }
    vec_available_shard_list.push_back(available_shard_list->at(i).asInt());
    }
    setAvailableShardList(vec_available_shard_list);
  }
  auto* follower_shard_list = data.get_ptr("FollowerShardList");
  if (follower_shard_list && follower_shard_list->isArray()) {
    std::vector<uint32_t> vec_follower_shard_list;
    for (size_t i = 0; i < follower_shard_list->size(); i++) {
    if (!follower_shard_list->at(i).isInt()) {
      return false;
    }
    vec_follower_shard_list.push_back(follower_shard_list->at(i).asInt());
    }
    setFollowerShardList(vec_follower_shard_list);
  }
  auto* follower_available_shard_list = data.get_ptr("FollowerAvailableShardList");
  if (follower_available_shard_list && follower_available_shard_list->isArray()) {
    std::vector<uint32_t> vec_follower_available_shard_list;
    for (size_t i = 0; i < follower_available_shard_list->size(); i++) {
    if (!follower_available_shard_list->at(i).isInt()) {
      return false;
    }
    vec_follower_available_shard_list.push_back(follower_available_shard_list->at(i).asInt());
    }
    setFollowerAvailableShardList(vec_follower_available_shard_list);
  }
  auto* other_settings = data.get_ptr("OtherSettings");
  if (other_settings == nullptr || !other_settings->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_other_settings;
  for (auto iter = other_settings->keys().begin(); iter != other_settings->keys().end(); iter++) {
    if (!other_settings->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_other_settings.insert({iter->asString(), other_settings->at(*iter).asString()});
  }
  setOtherSettings(map_other_settings);
  auto* is_edge_node = data.get_ptr("IsEdgeNode");
  if (is_edge_node && is_edge_node->isBool()) {
    setIsEdgeNode(is_edge_node->asBool());
  }
  auto* partition_list = data.get_ptr("PartitionList");
  if (partition_list && partition_list->isArray()) {
    std::vector<int64_t> vec_partition_list;
    for (size_t i = 0; i < partition_list->size(); i++) {
    if (!partition_list->at(i).isInt()) {
      return false;
    }
    vec_partition_list.push_back(partition_list->at(i).asInt());
    }
    setPartitionList(vec_partition_list);
  }

  return true;
}

void ServiceRouterConfig::describe(std::ostream& os) const {
  os << "ServiceRouterConfig{"
     << "TtlInMs=" << ttl_in_ms_
     << ", " << "LoadBalance=" << load_balance_
     << ", " << "TotalShards=" << total_shards_
     << ", " << "PullInterval=" << pull_interval_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ServiceRouterConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ServiceRouterConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("TtlInMs", ttl_in_ms_);
  folly::dynamic load_balance = serializeLoadBalanceMethod(load_balance_);
  result.insert("LoadBalance", load_balance);
  result.insert("TotalShards", total_shards_);
  result.insert("PullInterval", pull_interval_);

  return result;
}

bool ServiceRouterConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* ttl_in_ms = data.get_ptr("TtlInMs");
  if (ttl_in_ms == nullptr || !ttl_in_ms->isInt()) {
    return false;
  }
  setTtlInMs(ttl_in_ms->asInt());
  auto* load_balance = data.get_ptr("LoadBalance");
  if (load_balance == nullptr) {
    return false;
  }

  LoadBalanceMethod itemload_balance;
  if (!deserializeLoadBalanceMethod(*load_balance, &itemload_balance)) {
    return false;
  }
  setLoadBalance(itemload_balance);
  auto* total_shards = data.get_ptr("TotalShards");
  if (total_shards == nullptr || !total_shards->isInt()) {
    return false;
  }
  setTotalShards(total_shards->asInt());
  auto* pull_interval = data.get_ptr("PullInterval");
  if (pull_interval == nullptr || !pull_interval->isInt()) {
    return false;
  }
  setPullInterval(pull_interval->asInt());

  return true;
}

void ServiceConfig::describe(std::ostream& os) const {
  os << "ServiceConfig{"
     << "Router=" << router_
     << ", " << "Configs={";
  for (auto& t : configs_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ServiceConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ServiceConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic router = folly::dynamic::object;
  router = router_.serialize();
  result.insert("Router", router);
  folly::dynamic configs = folly::dynamic::object;
  for (auto& t : configs_) {
    configs.insert(t.first, t.second);
  }
  result.insert("Configs", configs);

  return result;
}

bool ServiceConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* router = data.get_ptr("Router");
  if (router == nullptr) {
    return false;
  }

  ServiceRouterConfig itemrouter;
  if (!itemrouter.deserialize(*router)) {
    return false;
  }
  setRouter(itemrouter);
  auto* configs = data.get_ptr("Configs");
  if (configs == nullptr || !configs->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_configs;
  for (auto iter = configs->keys().begin(); iter != configs->keys().end(); iter++) {
    if (!configs->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_configs.insert({iter->asString(), configs->at(*iter).asString()});
  }
  setConfigs(map_configs);

  return true;
}

void Service::describe(std::ostream& os) const {
  os << "Service{"
     << "Config=" << config_
     << ", " << "Nodes={";
  for (auto& t : nodes_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const Service& value) {
  value.describe(os);
  return os;
}

const folly::dynamic Service::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic config = folly::dynamic::object;
  config = config_.serialize();
  result.insert("Config", config);
  folly::dynamic nodes = folly::dynamic::object;
  for (auto& t : nodes_) {
    folly::dynamic map_nodes = folly::dynamic::object;
    map_nodes = t.second.serialize();
    nodes.insert(t.first, map_nodes);
  }
  result.insert("Nodes", nodes);

  return result;
}

bool Service::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* config = data.get_ptr("Config");
  if (config == nullptr) {
    return false;
  }

  ServiceConfig itemconfig;
  if (!itemconfig.deserialize(*config)) {
    return false;
  }
  setConfig(itemconfig);
  auto* nodes = data.get_ptr("Nodes");
  if (nodes == nullptr || !nodes->isObject()) {
    return false;
  }

  std::unordered_map<std::string, Server> map_nodes;
  for (auto iter = nodes->keys().begin(); iter != nodes->keys().end(); iter++) {
    if (!nodes->at(*iter).isObject()) {
      return false;
    }

    Server obj_nodes;
    if (!obj_nodes.deserialize(nodes->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_nodes.insert({iter->asString(), obj_nodes});
  }
  setNodes(map_nodes);

  return true;
}

void ThriftRetryOption::describe(std::ostream& os) const {
  os << "ThriftRetryOption{"
     << "ConnectionRetry=" << connection_retry_
     << ", " << "TimeoutRetry=" << timeout_retry_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ThriftRetryOption& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ThriftRetryOption::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("ConnectionRetry", connection_retry_);
  result.insert("TimeoutRetry", timeout_retry_);

  return result;
}

bool ThriftRetryOption::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* connection_retry = data.get_ptr("ConnectionRetry");
  if (connection_retry == nullptr || !connection_retry->isInt()) {
    return false;
  }
  setConnectionRetry(connection_retry->asInt());
  auto* timeout_retry = data.get_ptr("TimeoutRetry");
  if (timeout_retry == nullptr || !timeout_retry->isInt()) {
    return false;
  }
  setTimeoutRetry(timeout_retry->asInt());

  return true;
}


}  // namespace service_router
