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
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "folly/dynamic.h"
#include "proxygen/lib/utils/Base64.h"

namespace service_router {


enum class ThriftTransport {
  HEADER,
  RSOCKET,
  HTTP2
};

folly::dynamic serializeThriftTransport(const ThriftTransport& value);

bool deserializeThriftTransport(const folly::dynamic& data, ThriftTransport* value);

std::ostream& operator<<(std::ostream& os, const ThriftTransport& value);

folly::Optional<ThriftTransport> stringToThriftTransport(const std::string& name);

const std::string toStringThriftTransport(const ThriftTransport& value);

enum class RegistryType {
  CONSUL,
  AGENT
};

folly::dynamic serializeRegistryType(const RegistryType& value);

bool deserializeRegistryType(const folly::dynamic& data, RegistryType* value);

std::ostream& operator<<(std::ostream& os, const RegistryType& value);

folly::Optional<RegistryType> stringToRegistryType(const std::string& name);

const std::string toStringRegistryType(const RegistryType& value);

enum class ServerStatus {
  AVAILABLE,
  UNAVAILABLE
};

folly::dynamic serializeServerStatus(const ServerStatus& value);

bool deserializeServerStatus(const folly::dynamic& data, ServerStatus* value);

std::ostream& operator<<(std::ostream& os, const ServerStatus& value);

folly::Optional<ServerStatus> stringToServerStatus(const std::string& name);

const std::string toStringServerStatus(const ServerStatus& value);

enum class ServerProtocol {
  HTTP,
  THRIFT,
  REDIS
};

folly::dynamic serializeServerProtocol(const ServerProtocol& value);

bool deserializeServerProtocol(const folly::dynamic& data, ServerProtocol* value);

std::ostream& operator<<(std::ostream& os, const ServerProtocol& value);

folly::Optional<ServerProtocol> stringToServerProtocol(const std::string& name);

const std::string toStringServerProtocol(const ServerProtocol& value);

enum class LoadBalanceMethod {
  RANDOM,
  ROUNDROBIN,
  LOCALFIRST,
  CONSISTENT,
  CONFIGURABLE_WEIGHT,
  ACTIVE_WEIGHT,
  USER_DEFINED,
  IPRANGEFIRST,
  STATIC_WEIGHT,
  IDCFIRST
};

folly::dynamic serializeLoadBalanceMethod(const LoadBalanceMethod& value);

bool deserializeLoadBalanceMethod(const folly::dynamic& data, LoadBalanceMethod* value);

std::ostream& operator<<(std::ostream& os, const LoadBalanceMethod& value);

folly::Optional<LoadBalanceMethod> stringToLoadBalanceMethod(const std::string& name);

const std::string toStringLoadBalanceMethod(const LoadBalanceMethod& value);

class ServerAddress {
 public:
  ServerAddress() = default;
  ~ServerAddress() = default;

  const std::string& getHost() const { return host_; } 

  void setHost(const std::string& host) { host_ = host; } 

  uint16_t getPort() const { return port_; } 

  void setPort(uint16_t port) { port_ = port; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string host_;
  uint16_t port_{0};
};

std::ostream& operator<<(std::ostream& os, const ServerAddress& value);

class BalanceLocalFirstConfig {
 public:
  BalanceLocalFirstConfig() = default;
  ~BalanceLocalFirstConfig() = default;

  const std::string& getLocalIp() const { return local_ip_; } 

  void setLocalIp(const std::string& local_ip) { local_ip_ = local_ip; } 

  int getDiffRange() const { return diff_range_; } 

  void setDiffRange(int diff_range) { diff_range_ = diff_range; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string local_ip_;
  int diff_range_{256};
};

std::ostream& operator<<(std::ostream& os, const BalanceLocalFirstConfig& value);

enum class ShardType {
  ALL,
  LEADER,
  FOLLOWER
};

folly::dynamic serializeShardType(const ShardType& value);

bool deserializeShardType(const folly::dynamic& data, ShardType* value);

std::ostream& operator<<(std::ostream& os, const ShardType& value);


class ClientOption {
 public:
  ClientOption() = default;
  ~ClientOption() = default;

  const std::string& getServiceName() const { return service_name_; } 

  void setServiceName(const std::string& service_name) { service_name_ = service_name; } 

  uint32_t getShardId() const { return shard_id_; } 

  void setShardId(uint32_t shard_id) { shard_id_ = shard_id; } 

  int64_t getPartitionHash() const { return partition_hash_; } 

  void setPartitionHash(int64_t partition_hash) { partition_hash_ = partition_hash; } 

  bool getRouteToEdgeNode() const { return route_to_edge_node_; } 

  void setRouteToEdgeNode(bool route_to_edge_node) { route_to_edge_node_ = route_to_edge_node; } 

  const ShardType& getShardType() const { return shard_type_; } 

  void setShardType(const ShardType& shard_type) { shard_type_ = shard_type; } 

  const ServerProtocol& getProtocol() const { return protocol_; } 

  void setProtocol(const ServerProtocol& protocol) { protocol_ = protocol; } 

  const LoadBalanceMethod& getLoadBalance() const { return load_balance_; } 

  void setLoadBalance(const LoadBalanceMethod& load_balance) { load_balance_ = load_balance; } 

  const ServerAddress& getTargetServerAddress() const { return target_server_address_; } 

  void setTargetServerAddress(const ServerAddress& target_server_address) { target_server_address_ = target_server_address; } 

  int getTimeoutMs() const { return timeout_ms_; } 

  void setTimeoutMs(int timeout_ms) { timeout_ms_ = timeout_ms; } 

  const std::string& getUserBalance() const { return user_balance_; } 

  void setUserBalance(const std::string& user_balance) { user_balance_ = user_balance; } 

  const BalanceLocalFirstConfig& getLocalFirstConfig() const { return local_first_config_; } 

  void setLocalFirstConfig(const BalanceLocalFirstConfig& local_first_config) { local_first_config_ = local_first_config; } 

  int getMaxConnPerServer() const { return max_conn_per_server_; } 

  void setMaxConnPerServer(int max_conn_per_server) { max_conn_per_server_ = max_conn_per_server; } 

  uint16_t getThriftCompressionMethod() const { return thrift_compression_method_; } 

  void setThriftCompressionMethod(uint16_t thrift_compression_method) { thrift_compression_method_ = thrift_compression_method; } 

  const std::string& getIdc() const { return idc_; } 

  void setIdc(const std::string& idc) { idc_ = idc; } 

  const std::string& getDc() const { return dc_; } 

  void setDc(const std::string& dc) { dc_ = dc; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string service_name_;
  uint32_t shard_id_{4294967295};
  int64_t partition_hash_{0};
  bool route_to_edge_node_{false};
  ShardType shard_type_{ShardType::LEADER};
  ServerProtocol protocol_;
  LoadBalanceMethod load_balance_;
  ServerAddress target_server_address_;
  int timeout_ms_{0};
  std::string user_balance_;
  BalanceLocalFirstConfig local_first_config_;
  int max_conn_per_server_{0};
  uint16_t thrift_compression_method_{3};
  std::string idc_;
  std::string dc_{"default"};
};

std::ostream& operator<<(std::ostream& os, const ClientOption& value);

class ServerOption {
 public:
  ServerOption() = default;
  ~ServerOption() = default;

  const std::string& getServiceName() const { return service_name_; } 

  void setServiceName(const std::string& service_name) { service_name_ = service_name; } 

  const ServerAddress& getServerAddress() const { return server_address_; } 

  void setServerAddress(const ServerAddress& server_address) { server_address_ = server_address; } 

  bool getReusePort() const { return reuse_port_; } 

  void setReusePort(bool reuse_port) { reuse_port_ = reuse_port; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string service_name_;
  ServerAddress server_address_;
  bool reuse_port_{false};
};

std::ostream& operator<<(std::ostream& os, const ServerOption& value);

class Server {
 public:
  Server() = default;
  ~Server() = default;

  const std::string& getHost() const { return host_; } 

  void setHost(const std::string& host) { host_ = host; } 

  uint16_t getPort() const { return port_; } 

  void setPort(uint16_t port) { port_ = port; } 

  const std::string& getServiceName() const { return service_name_; } 

  void setServiceName(const std::string& service_name) { service_name_ = service_name; } 

  const ServerProtocol& getProtocol() const { return protocol_; } 

  void setProtocol(const ServerProtocol& protocol) { protocol_ = protocol; } 

  const ServerStatus& getStatus() const { return status_; } 

  void setStatus(const ServerStatus& status) { status_ = status; } 

  const std::string& getIdc() const { return idc_; } 

  void setIdc(const std::string& idc) { idc_ = idc; } 

  const std::string& getDc() const { return dc_; } 

  void setDc(const std::string& dc) { dc_ = dc; } 

  uint64_t getUpdateTime() const { return update_time_; } 

  void setUpdateTime(uint64_t update_time) { update_time_ = update_time; } 

  int getWeight() const { return weight_; } 

  void setWeight(int weight) { weight_ = weight; } 

  uint32_t getHostLong() const { return host_long_; } 

  void setHostLong(uint32_t host_long) { host_long_ = host_long; } 

  const std::vector<uint32_t>& getShardList() const { return shard_list_; } 

  void setShardList(const std::vector<uint32_t>& shard_list) { shard_list_ = shard_list; } 

  const std::vector<uint32_t>& getAvailableShardList() const { return available_shard_list_; } 

  void setAvailableShardList(const std::vector<uint32_t>& available_shard_list) { available_shard_list_ = available_shard_list; } 

  const std::vector<uint32_t>& getFollowerShardList() const { return follower_shard_list_; } 

  void setFollowerShardList(const std::vector<uint32_t>& follower_shard_list) { follower_shard_list_ = follower_shard_list; } 

  const std::vector<uint32_t>& getFollowerAvailableShardList() const { return follower_available_shard_list_; } 

  void setFollowerAvailableShardList(const std::vector<uint32_t>& follower_available_shard_list) { follower_available_shard_list_ = follower_available_shard_list; } 

  const std::unordered_map<std::string, std::string>& getOtherSettings() const { return other_settings_; } 

  void setOtherSettings(const std::unordered_map<std::string, std::string>& other_settings) { other_settings_ = other_settings; } 

  bool getIsEdgeNode() const { return is_edge_node_; } 

  void setIsEdgeNode(bool is_edge_node) { is_edge_node_ = is_edge_node; } 

  const std::vector<int64_t>& getPartitionList() const { return partition_list_; } 

  void setPartitionList(const std::vector<int64_t>& partition_list) { partition_list_ = partition_list; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string host_;
  uint16_t port_;
  std::string service_name_;
  ServerProtocol protocol_{ServerProtocol::THRIFT};
  ServerStatus status_{ServerStatus::UNAVAILABLE};
  std::string idc_;
  std::string dc_{"default"};
  uint64_t update_time_{0};
  int weight_{1};
  uint32_t host_long_{0};
  std::vector<uint32_t> shard_list_;
  std::vector<uint32_t> available_shard_list_;
  std::vector<uint32_t> follower_shard_list_;
  std::vector<uint32_t> follower_available_shard_list_;
  std::unordered_map<std::string, std::string> other_settings_;
  bool is_edge_node_{false};
  std::vector<int64_t> partition_list_;
};

std::ostream& operator<<(std::ostream& os, const Server& value);

class ServiceRouterConfig {
 public:
  ServiceRouterConfig() = default;
  ~ServiceRouterConfig() = default;

  int getTtlInMs() const { return ttl_in_ms_; } 

  void setTtlInMs(int ttl_in_ms) { ttl_in_ms_ = ttl_in_ms; } 

  const LoadBalanceMethod& getLoadBalance() const { return load_balance_; } 

  void setLoadBalance(const LoadBalanceMethod& load_balance) { load_balance_ = load_balance; } 

  int getTotalShards() const { return total_shards_; } 

  void setTotalShards(int total_shards) { total_shards_ = total_shards; } 

  int getPullInterval() const { return pull_interval_; } 

  void setPullInterval(int pull_interval) { pull_interval_ = pull_interval; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  int ttl_in_ms_{3000};
  LoadBalanceMethod load_balance_{LoadBalanceMethod::RANDOM};
  int total_shards_{0};
  int pull_interval_{3};
};

std::ostream& operator<<(std::ostream& os, const ServiceRouterConfig& value);

class ServiceConfig {
 public:
  ServiceConfig() = default;
  ~ServiceConfig() = default;

  const ServiceRouterConfig& getRouter() const { return router_; } 

  void setRouter(const ServiceRouterConfig& router) { router_ = router; } 

  const std::unordered_map<std::string, std::string>& getConfigs() const { return configs_; } 

  void setConfigs(const std::unordered_map<std::string, std::string>& configs) { configs_ = configs; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  ServiceRouterConfig router_;
  std::unordered_map<std::string, std::string> configs_;
};

std::ostream& operator<<(std::ostream& os, const ServiceConfig& value);

class Service {
 public:
  Service() = default;
  ~Service() = default;

  const ServiceConfig& getConfig() const { return config_; } 

  void setConfig(const ServiceConfig& config) { config_ = config; } 

  const std::unordered_map<std::string, Server>& getNodes() const { return nodes_; } 

  void setNodes(const std::unordered_map<std::string, Server>& nodes) { nodes_ = nodes; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  ServiceConfig config_;
  std::unordered_map<std::string, Server> nodes_;
};

std::ostream& operator<<(std::ostream& os, const Service& value);

class ThriftRetryOption {
 public:
  ThriftRetryOption() = default;
  ~ThriftRetryOption() = default;

  int getConnectionRetry() const { return connection_retry_; } 

  void setConnectionRetry(int connection_retry) { connection_retry_ = connection_retry; } 

  int getTimeoutRetry() const { return timeout_retry_; } 

  void setTimeoutRetry(int timeout_retry) { timeout_retry_ = timeout_retry; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  int connection_retry_{0};
  int timeout_retry_{0};
};

std::ostream& operator<<(std::ostream& os, const ThriftRetryOption& value);


}  // namespace service_router
