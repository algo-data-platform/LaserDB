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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "folly/dynamic.h"
#include "proxygen/lib/utils/Base64.h"

namespace laser {


class RocksDbConfig {
 public:
  RocksDbConfig() = default;
  ~RocksDbConfig() = default;

  const std::unordered_map<std::string, std::string>& getDbOptions() const { return db_options_; } 

  void setDbOptions(const std::unordered_map<std::string, std::string>& db_options) { db_options_ = db_options; } 

  const std::unordered_map<std::string, std::string>& getCfOptions() const { return cf_options_; } 

  void setCfOptions(const std::unordered_map<std::string, std::string>& cf_options) { cf_options_ = cf_options; } 

  const std::unordered_map<std::string, std::string>& getTableOptions() const { return table_options_; } 

  void setTableOptions(const std::unordered_map<std::string, std::string>& table_options) { table_options_ = table_options; } 

  uint32_t getBlockCacheSizeGb() const { return block_cache_size_gb_; } 

  void setBlockCacheSizeGb(uint32_t block_cache_size_gb) { block_cache_size_gb_ = block_cache_size_gb; } 

  uint32_t getWriteBufferSizeGb() const { return write_buffer_size_gb_; } 

  void setWriteBufferSizeGb(uint32_t write_buffer_size_gb) { write_buffer_size_gb_ = write_buffer_size_gb; } 

  int32_t getNumShardBits() const { return num_shard_bits_; } 

  void setNumShardBits(int32_t num_shard_bits) { num_shard_bits_ = num_shard_bits; } 

  double getHighPriPoolRatio() const { return high_pri_pool_ratio_; } 

  void setHighPriPoolRatio(double high_pri_pool_ratio) { high_pri_pool_ratio_ = high_pri_pool_ratio; } 

  bool getStrictCapacityLimit() const { return strict_capacity_limit_; } 

  void setStrictCapacityLimit(bool strict_capacity_limit) { strict_capacity_limit_ = strict_capacity_limit; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, std::string> db_options_;
  std::unordered_map<std::string, std::string> cf_options_;
  std::unordered_map<std::string, std::string> table_options_;
  uint32_t block_cache_size_gb_{2};
  uint32_t write_buffer_size_gb_{1};
  int32_t num_shard_bits_{-1};
  double high_pri_pool_ratio_{0.0};
  bool strict_capacity_limit_{false};
};

std::ostream& operator<<(std::ostream& os, const RocksDbConfig& value);

class NodeRateLimitEntry {
 public:
  NodeRateLimitEntry() = default;
  ~NodeRateLimitEntry() = default;

  uint32_t getBeginHour() const { return begin_hour_; } 

  void setBeginHour(uint32_t begin_hour) { begin_hour_ = begin_hour; } 

  uint32_t getEndHour() const { return end_hour_; } 

  void setEndHour(uint32_t end_hour) { end_hour_ = end_hour; } 

  int64_t getRateBytesPerSec() const { return rate_bytes_per_sec_; } 

  void setRateBytesPerSec(int64_t rate_bytes_per_sec) { rate_bytes_per_sec_ = rate_bytes_per_sec; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t begin_hour_{0};
  uint32_t end_hour_{0};
  int64_t rate_bytes_per_sec_{0};
};

std::ostream& operator<<(std::ostream& os, const NodeRateLimitEntry& value);

class NodeConfig {
 public:
  NodeConfig() = default;
  ~NodeConfig() = default;

  uint32_t getBlockCacheSizeGb() const { return block_cache_size_gb_; } 

  void setBlockCacheSizeGb(uint32_t block_cache_size_gb) { block_cache_size_gb_ = block_cache_size_gb; } 

  uint32_t getWriteBufferSizeGb() const { return write_buffer_size_gb_; } 

  void setWriteBufferSizeGb(uint32_t write_buffer_size_gb) { write_buffer_size_gb_ = write_buffer_size_gb; } 

  int32_t getNumShardBits() const { return num_shard_bits_; } 

  void setNumShardBits(int32_t num_shard_bits) { num_shard_bits_ = num_shard_bits; } 

  double getHighPriPoolRatio() const { return high_pri_pool_ratio_; } 

  void setHighPriPoolRatio(double high_pri_pool_ratio) { high_pri_pool_ratio_ = high_pri_pool_ratio; } 

  bool getStrictCapacityLimit() const { return strict_capacity_limit_; } 

  void setStrictCapacityLimit(bool strict_capacity_limit) { strict_capacity_limit_ = strict_capacity_limit; } 

  const std::vector<NodeRateLimitEntry>& getRateLimitStrategy() const { return rate_limit_strategy_; } 

  void setRateLimitStrategy(const std::vector<NodeRateLimitEntry>& rate_limit_strategy) { rate_limit_strategy_ = rate_limit_strategy; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t block_cache_size_gb_{2};
  uint32_t write_buffer_size_gb_{1};
  int32_t num_shard_bits_{-1};
  double high_pri_pool_ratio_{0.0};
  bool strict_capacity_limit_{false};
  std::vector<NodeRateLimitEntry> rate_limit_strategy_;
};

std::ostream& operator<<(std::ostream& os, const NodeConfig& value);

class TableConfig {
 public:
  TableConfig() = default;
  ~TableConfig() = default;

  const std::unordered_map<std::string, std::string>& getDbOptions() const { return db_options_; } 

  void setDbOptions(const std::unordered_map<std::string, std::string>& db_options) { db_options_ = db_options; } 

  const std::unordered_map<std::string, std::string>& getCfOptions() const { return cf_options_; } 

  void setCfOptions(const std::unordered_map<std::string, std::string>& cf_options) { cf_options_ = cf_options; } 

  const std::unordered_map<std::string, std::string>& getTableOptions() const { return table_options_; } 

  void setTableOptions(const std::unordered_map<std::string, std::string>& table_options) { table_options_ = table_options; } 

  uint32_t getVersion() const { return version_; } 

  void setVersion(uint32_t version) { version_ = version; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, std::string> db_options_;
  std::unordered_map<std::string, std::string> cf_options_;
  std::unordered_map<std::string, std::string> table_options_;
  uint32_t version_;
};

std::ostream& operator<<(std::ostream& os, const TableConfig& value);

class ProxySpecificTableConfig {
 public:
  ProxySpecificTableConfig() = default;
  ~ProxySpecificTableConfig() = default;

  uint32_t getTableReadTimeout() const { return table_read_timeout_; } 

  void setTableReadTimeout(uint32_t table_read_timeout) { table_read_timeout_ = table_read_timeout; } 

  uint32_t getTableWriteTimeout() const { return table_write_timeout_; } 

  void setTableWriteTimeout(uint32_t table_write_timeout) { table_write_timeout_ = table_write_timeout; } 

  uint32_t getTableAllowedFlow() const { return table_allowed_flow_; } 

  void setTableAllowedFlow(uint32_t table_allowed_flow) { table_allowed_flow_ = table_allowed_flow; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t table_read_timeout_{10};
  uint32_t table_write_timeout_{10};
  uint32_t table_allowed_flow_{100};
};

std::ostream& operator<<(std::ostream& os, const ProxySpecificTableConfig& value);

class ProxyDatabaseTableConfig {
 public:
  ProxyDatabaseTableConfig() = default;
  ~ProxyDatabaseTableConfig() = default;

  const std::unordered_map<std::string, ProxySpecificTableConfig>& getProxyDbTableConfig() const { return proxy_db_table_config_; } 

  void setProxyDbTableConfig(const std::unordered_map<std::string, ProxySpecificTableConfig>& proxy_db_table_config) { proxy_db_table_config_ = proxy_db_table_config; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, ProxySpecificTableConfig> proxy_db_table_config_;
};

std::ostream& operator<<(std::ostream& os, const ProxyDatabaseTableConfig& value);

class ProxyDatabaseTableConfigList {
 public:
  ProxyDatabaseTableConfigList() = default;
  ~ProxyDatabaseTableConfigList() = default;

  const std::unordered_map<std::string, ProxyDatabaseTableConfig>& getProxyDbTableConfigList() const { return proxy_db_table_config_list_; } 

  void setProxyDbTableConfigList(const std::unordered_map<std::string, ProxyDatabaseTableConfig>& proxy_db_table_config_list) { proxy_db_table_config_list_ = proxy_db_table_config_list; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, ProxyDatabaseTableConfig> proxy_db_table_config_list_;
};

std::ostream& operator<<(std::ostream& os, const ProxyDatabaseTableConfigList& value);

class ProxyTableConfig {
 public:
  ProxyTableConfig() = default;
  ~ProxyTableConfig() = default;

  uint32_t getLaserClientReadTimeout() const { return laser_client_read_timeout_; } 

  void setLaserClientReadTimeout(uint32_t laser_client_read_timeout) { laser_client_read_timeout_ = laser_client_read_timeout; } 

  uint32_t getLaserClientWriteTimeout() const { return laser_client_write_timeout_; } 

  void setLaserClientWriteTimeout(uint32_t laser_client_write_timeout) { laser_client_write_timeout_ = laser_client_write_timeout; } 

  uint32_t getLaserClientAllowedFlow() const { return laser_client_allowed_flow_; } 

  void setLaserClientAllowedFlow(uint32_t laser_client_allowed_flow) { laser_client_allowed_flow_ = laser_client_allowed_flow; } 

  ProxyDatabaseTableConfigList getProxyTableConfig() const { return proxy_table_config_; } 

  void setProxyTableConfig(ProxyDatabaseTableConfigList proxy_table_config) { proxy_table_config_ = proxy_table_config; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t laser_client_read_timeout_{10};
  uint32_t laser_client_write_timeout_{10};
  uint32_t laser_client_allowed_flow_{100};
  ProxyDatabaseTableConfigList proxy_table_config_;
};

std::ostream& operator<<(std::ostream& os, const ProxyTableConfig& value);

class NodeConfigList {
 public:
  NodeConfigList() = default;
  ~NodeConfigList() = default;

  const std::unordered_map<std::string, NodeConfig>& getNodeConfigList() const { return node_config_list_; } 

  void setNodeConfigList(const std::unordered_map<std::string, NodeConfig>& node_config_list) { node_config_list_ = node_config_list; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, NodeConfig> node_config_list_;
};

std::ostream& operator<<(std::ostream& os, const NodeConfigList& value);

class TableConfigList {
 public:
  TableConfigList() = default;
  ~TableConfigList() = default;

  const std::unordered_map<std::string, TableConfig>& getTableConfigList() const { return table_config_list_; } 

  void setTableConfigList(const std::unordered_map<std::string, TableConfig>& table_config_list) { table_config_list_ = table_config_list; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, TableConfig> table_config_list_;
};

std::ostream& operator<<(std::ostream& os, const TableConfigList& value);

class RocksdbNodeConfigs {
 public:
  RocksdbNodeConfigs() = default;
  ~RocksdbNodeConfigs() = default;

  const std::unordered_map<std::string, std::string>& getRocksdbNodeConfigs() const { return rocksdb_node_configs_; } 

  void setRocksdbNodeConfigs(const std::unordered_map<std::string, std::string>& rocksdb_node_configs) { rocksdb_node_configs_ = rocksdb_node_configs; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::unordered_map<std::string, std::string> rocksdb_node_configs_;
};

std::ostream& operator<<(std::ostream& os, const RocksdbNodeConfigs& value);

enum class TrafficRestrictionType {
  KPS,
  QPS
};

folly::dynamic serializeTrafficRestrictionType(const TrafficRestrictionType& value);

bool deserializeTrafficRestrictionType(const folly::dynamic& data, TrafficRestrictionType* value);

std::ostream& operator<<(std::ostream& os, const TrafficRestrictionType& value);


class TrafficRestrictionLimitItem {
 public:
  TrafficRestrictionLimitItem() = default;
  ~TrafficRestrictionLimitItem() = default;

  const TrafficRestrictionType& getType() const { return type_; } 

  void setType(const TrafficRestrictionType& type) { type_ = type; } 

  uint32_t getLimit() const { return limit_; } 

  void setLimit(uint32_t limit) { limit_ = limit; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  TrafficRestrictionType type_;
  uint32_t limit_;
};

std::ostream& operator<<(std::ostream& os, const TrafficRestrictionLimitItem& value);

class TableTrafficRestrictionConfig {
 public:
  TableTrafficRestrictionConfig() = default;
  ~TableTrafficRestrictionConfig() = default;

  const std::string& getTableName() const { return table_name_; } 

  void setTableName(const std::string& table_name) { table_name_ = table_name; } 

  bool getDenyAll() const { return deny_all_; } 

  void setDenyAll(bool deny_all) { deny_all_ = deny_all; } 

  const std::unordered_map<std::string, uint32_t>& getSingleOperationLimits() const { return single_operation_limits_; } 

  void setSingleOperationLimits(const std::unordered_map<std::string, uint32_t>& single_operation_limits) { single_operation_limits_ = single_operation_limits; } 

  const std::unordered_map<std::string, TrafficRestrictionLimitItem>& getMultipleOperationLimits() const { return multiple_operation_limits_; } 

  void setMultipleOperationLimits(const std::unordered_map<std::string, TrafficRestrictionLimitItem>& multiple_operation_limits) { multiple_operation_limits_ = multiple_operation_limits; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string table_name_;
  bool deny_all_{false};
  std::unordered_map<std::string, uint32_t> single_operation_limits_;
  std::unordered_map<std::string, TrafficRestrictionLimitItem> multiple_operation_limits_;
};

std::ostream& operator<<(std::ostream& os, const TableTrafficRestrictionConfig& value);

class DatabaseTrafficRestrictionConfig {
 public:
  DatabaseTrafficRestrictionConfig() = default;
  ~DatabaseTrafficRestrictionConfig() = default;

  const std::string& getDatabaseName() const { return database_name_; } 

  void setDatabaseName(const std::string& database_name) { database_name_ = database_name; } 

  std::vector<TableTrafficRestrictionConfig> getTables() const { return tables_; } 

  void setTables(std::vector<TableTrafficRestrictionConfig> tables) { tables_ = tables; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string database_name_;
  std::vector<TableTrafficRestrictionConfig> tables_;
};

std::ostream& operator<<(std::ostream& os, const DatabaseTrafficRestrictionConfig& value);

enum class TableLoaderType {
  BASE,
  DELTA,
  STREAM
};

folly::dynamic serializeTableLoaderType(const TableLoaderType& value);

bool deserializeTableLoaderType(const folly::dynamic& data, TableLoaderType* value);

std::ostream& operator<<(std::ostream& os, const TableLoaderType& value);

folly::Optional<TableLoaderType> stringToTableLoaderType(const std::string& name);

const std::string toStringTableLoaderType(const TableLoaderType& value);

enum class TableLoaderSourceType {
  HDFS,
  KAFKA,
  LOCAL
};

folly::dynamic serializeTableLoaderSourceType(const TableLoaderSourceType& value);

bool deserializeTableLoaderSourceType(const folly::dynamic& data, TableLoaderSourceType* value);

std::ostream& operator<<(std::ostream& os, const TableLoaderSourceType& value);

folly::Optional<TableLoaderSourceType> stringToTableLoaderSourceType(const std::string& name);

const std::string toStringTableLoaderSourceType(const TableLoaderSourceType& value);

class TableDataLoaderSchema {
 public:
  TableDataLoaderSchema() = default;
  ~TableDataLoaderSchema() = default;

  const TableLoaderType& getType() const { return type_; } 

  void setType(const TableLoaderType& type) { type_ = type; } 

  const TableLoaderSourceType& getSourceType() const { return source_type_; } 

  void setSourceType(const TableLoaderSourceType& source_type) { source_type_ = source_type; } 

  const std::unordered_map<std::string, std::string>& getParams() const { return params_; } 

  void setParams(const std::unordered_map<std::string, std::string>& params) { params_ = params; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  TableLoaderType type_;
  TableLoaderSourceType source_type_;
  std::unordered_map<std::string, std::string> params_;
};

std::ostream& operator<<(std::ostream& os, const TableDataLoaderSchema& value);

enum class TableEngineType {
  ROCKSDB,
  UNORDERED_MAP
};

folly::dynamic serializeTableEngineType(const TableEngineType& value);

bool deserializeTableEngineType(const folly::dynamic& data, TableEngineType* value);

std::ostream& operator<<(std::ostream& os, const TableEngineType& value);

folly::Optional<TableEngineType> stringToTableEngineType(const std::string& name);

const std::string toStringTableEngineType(const TableEngineType& value);

class TableSchema {
 public:
  TableSchema() = default;
  ~TableSchema() = default;

  const std::string& getDatabaseName() const { return database_name_; } 

  void setDatabaseName(const std::string& database_name) { database_name_ = database_name; } 

  const std::string& getTableName() const { return table_name_; } 

  void setTableName(const std::string& table_name) { table_name_ = table_name; } 

  int getPartitionNumber() const { return partition_number_; } 

  void setPartitionNumber(int partition_number) { partition_number_ = partition_number; } 

  uint64_t getTtl() const { return ttl_; } 

  void setTtl(uint64_t ttl) { ttl_ = ttl; } 

  const std::string& getConfigName() const { return config_name_; } 

  void setConfigName(const std::string& config_name) { config_name_ = config_name; } 

  const std::vector<std::string>& getBindEdgeNodes() const { return bind_edge_nodes_; } 

  void setBindEdgeNodes(const std::vector<std::string>& bind_edge_nodes) { bind_edge_nodes_ = bind_edge_nodes; } 

  int getEdgeFlowRatio() const { return edge_flow_ratio_; } 

  void setEdgeFlowRatio(int edge_flow_ratio) { edge_flow_ratio_ = edge_flow_ratio; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string database_name_;
  std::string table_name_;
  int partition_number_;
  uint64_t ttl_{0};
  std::string config_name_;
  std::vector<std::string> bind_edge_nodes_;
  int edge_flow_ratio_{0};
};

std::ostream& operator<<(std::ostream& os, const TableSchema& value);

class DatabaseSchema {
 public:
  DatabaseSchema() = default;
  ~DatabaseSchema() = default;

  const std::string& getDatabaseName() const { return database_name_; } 

  void setDatabaseName(const std::string& database_name) { database_name_ = database_name; } 

  std::vector<TableSchema> getTables() const { return tables_; } 

  void setTables(std::vector<TableSchema> tables) { tables_ = tables; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string database_name_;
  std::vector<TableSchema> tables_;
};

std::ostream& operator<<(std::ostream& os, const DatabaseSchema& value);

class NodeShardList {
 public:
  NodeShardList() = default;
  ~NodeShardList() = default;

  uint32_t getNodeId() const { return node_id_; } 

  void setNodeId(uint32_t node_id) { node_id_ = node_id; } 

  const std::vector<uint32_t>& getLeaderShardList() const { return leader_shard_list_; } 

  void setLeaderShardList(const std::vector<uint32_t>& leader_shard_list) { leader_shard_list_ = leader_shard_list; } 

  const std::vector<uint32_t>& getFollowerShardList() const { return follower_shard_list_; } 

  void setFollowerShardList(const std::vector<uint32_t>& follower_shard_list) { follower_shard_list_ = follower_shard_list; } 

  bool getIsEdgeNode() const { return is_edge_node_; } 

  void setIsEdgeNode(bool is_edge_node) { is_edge_node_ = is_edge_node; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t node_id_;
  std::vector<uint32_t> leader_shard_list_;
  std::vector<uint32_t> follower_shard_list_;
  bool is_edge_node_{false};
};

std::ostream& operator<<(std::ostream& os, const NodeShardList& value);

class ClusterGroup {
 public:
  ClusterGroup() = default;
  ~ClusterGroup() = default;

  const std::string& getGroupName() const { return group_name_; } 

  void setGroupName(const std::string& group_name) { group_name_ = group_name; } 

  const std::vector<NodeShardList>& getNodes() const { return nodes_; } 

  void setNodes(const std::vector<NodeShardList>& nodes) { nodes_ = nodes; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string group_name_;
  std::vector<NodeShardList> nodes_;
};

std::ostream& operator<<(std::ostream& os, const ClusterGroup& value);

class ClusterInfo {
 public:
  ClusterInfo() = default;
  ~ClusterInfo() = default;

  const std::string& getClusterName() const { return cluster_name_; } 

  void setClusterName(const std::string& cluster_name) { cluster_name_ = cluster_name; } 

  uint32_t getShardNumber() const { return shard_number_; } 

  void setShardNumber(uint32_t shard_number) { shard_number_ = shard_number; } 

  const std::vector<ClusterGroup>& getGroups() const { return groups_; } 

  void setGroups(const std::vector<ClusterGroup>& groups) { groups_ = groups; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string cluster_name_;
  uint32_t shard_number_;
  std::vector<ClusterGroup> groups_;
};

std::ostream& operator<<(std::ostream& os, const ClusterInfo& value);

enum class DBRole {
  LEADER,
  FOLLOWER
};

folly::dynamic serializeDBRole(const DBRole& value);

bool deserializeDBRole(const folly::dynamic& data, DBRole* value);

std::ostream& operator<<(std::ostream& os, const DBRole& value);

folly::Optional<DBRole> stringToDBRole(const std::string& name);

const std::string toStringDBRole(const DBRole& value);

class ReplicationDbMetaInfo {
 public:
  ReplicationDbMetaInfo() = default;
  ~ReplicationDbMetaInfo() = default;

  uint64_t getSeqNo() const { return seq_no_; } 

  void setSeqNo(uint64_t seq_no) { seq_no_ = seq_no; } 

  int64_t getReplicateLag() const { return replicate_lag_; } 

  void setReplicateLag(int64_t replicate_lag) { replicate_lag_ = replicate_lag; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint64_t seq_no_{0};
  int64_t replicate_lag_{0};
};

std::ostream& operator<<(std::ostream& os, const ReplicationDbMetaInfo& value);

class PartitionMetaInfo {
 public:
  PartitionMetaInfo() = default;
  ~PartitionMetaInfo() = default;

  const DBRole& getRole() const { return role_; } 

  void setRole(const DBRole& role) { role_ = role; } 

  uint32_t getPartitionId() const { return partition_id_; } 

  void setPartitionId(uint32_t partition_id) { partition_id_ = partition_id; } 

  uint64_t getHash() const { return hash_; } 

  void setHash(uint64_t hash) { hash_ = hash; } 

  uint64_t getSize() const { return size_; } 

  void setSize(uint64_t size) { size_ = size; } 

  uint64_t getReadKps() const { return read_kps_; } 

  void setReadKps(uint64_t read_kps) { read_kps_ = read_kps; } 

  uint64_t getWriteKps() const { return write_kps_; } 

  void setWriteKps(uint64_t write_kps) { write_kps_ = write_kps; } 

  uint64_t getReadBytes() const { return read_bytes_; } 

  void setReadBytes(uint64_t read_bytes) { read_bytes_ = read_bytes; } 

  uint64_t getWriteBytes() const { return write_bytes_; } 

  void setWriteBytes(uint64_t write_bytes) { write_bytes_ = write_bytes; } 

  const std::string& getDatabaseName() const { return database_name_; } 

  void setDatabaseName(const std::string& database_name) { database_name_ = database_name; } 

  const std::string& getTableName() const { return table_name_; } 

  void setTableName(const std::string& table_name) { table_name_ = table_name; } 

  const std::string& getBaseVersion() const { return base_version_; } 

  void setBaseVersion(const std::string& base_version) { base_version_ = base_version; } 

  const std::vector<std::string>& getDeltaVersions() const { return delta_versions_; } 

  void setDeltaVersions(const std::vector<std::string>& delta_versions) { delta_versions_ = delta_versions; } 

  const ReplicationDbMetaInfo& getDbInfo() const { return db_info_; } 

  void setDbInfo(const ReplicationDbMetaInfo& db_info) { db_info_ = db_info; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  DBRole role_;
  uint32_t partition_id_;
  uint64_t hash_;
  uint64_t size_;
  uint64_t read_kps_;
  uint64_t write_kps_;
  uint64_t read_bytes_;
  uint64_t write_bytes_;
  std::string database_name_;
  std::string table_name_;
  std::string base_version_;
  std::vector<std::string> delta_versions_;
  ReplicationDbMetaInfo db_info_;
};

std::ostream& operator<<(std::ostream& os, const PartitionMetaInfo& value);

class TableMetaInfo {
 public:
  TableMetaInfo() = default;
  ~TableMetaInfo() = default;

  const std::string& getGroupName() const { return group_name_; } 

  void setGroupName(const std::string& group_name) { group_name_ = group_name; } 

  uint32_t getNodeId() const { return node_id_; } 

  void setNodeId(uint32_t node_id) { node_id_ = node_id; } 

  const std::vector<PartitionMetaInfo>& getPartitions() const { return partitions_; } 

  void setPartitions(const std::vector<PartitionMetaInfo>& partitions) { partitions_ = partitions; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::string group_name_;
  uint32_t node_id_;
  std::vector<PartitionMetaInfo> partitions_;
};

std::ostream& operator<<(std::ostream& os, const TableMetaInfo& value);

enum class ShardServiceStatus {
  AVAILABLE,
  UNAVAILABLE
};

folly::dynamic serializeShardServiceStatus(const ShardServiceStatus& value);

bool deserializeShardServiceStatus(const folly::dynamic& data, ShardServiceStatus* value);

std::ostream& operator<<(std::ostream& os, const ShardServiceStatus& value);

folly::Optional<ShardServiceStatus> stringToShardServiceStatus(const std::string& name);

const std::string toStringShardServiceStatus(const ShardServiceStatus& value);

class ShardMetaInfo {
 public:
  ShardMetaInfo() = default;
  ~ShardMetaInfo() = default;

  uint32_t getShardId() const { return shard_id_; } 

  void setShardId(uint32_t shard_id) { shard_id_ = shard_id; } 

  const DBRole& getRole() const { return role_; } 

  void setRole(const DBRole& role) { role_ = role; } 

  const ShardServiceStatus& getStatus() const { return status_; } 

  void setStatus(const ShardServiceStatus& status) { status_ = status; } 

  const std::vector<PartitionMetaInfo>& getPartitions() const { return partitions_; } 

  void setPartitions(const std::vector<PartitionMetaInfo>& partitions) { partitions_ = partitions; } 

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  uint32_t shard_id_;
  DBRole role_;
  ShardServiceStatus status_;
  std::vector<PartitionMetaInfo> partitions_;
};

std::ostream& operator<<(std::ostream& os, const ShardMetaInfo& value);


}  // namespace laser
