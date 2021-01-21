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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#include "laser_entity.h"

namespace laser {


void RocksDbConfig::describe(std::ostream& os) const {
  os << "RocksDbConfig{"
     << "DbOptions={";
  for (auto& t : db_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "CfOptions={";
  for (auto& t : cf_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "TableOptions={";
  for (auto& t : table_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "BlockCacheSizeGb=" << block_cache_size_gb_
     << ", " << "WriteBufferSizeGb=" << write_buffer_size_gb_
     << ", " << "NumShardBits=" << num_shard_bits_
     << ", " << "HighPriPoolRatio=" << high_pri_pool_ratio_
     << ", " << "StrictCapacityLimit=" << strict_capacity_limit_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const RocksDbConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic RocksDbConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic db_options = folly::dynamic::object;
  for (auto& t : db_options_) {
    db_options.insert(t.first, t.second);
  }
  result.insert("DbOptions", db_options);
  folly::dynamic cf_options = folly::dynamic::object;
  for (auto& t : cf_options_) {
    cf_options.insert(t.first, t.second);
  }
  result.insert("CfOptions", cf_options);
  folly::dynamic table_options = folly::dynamic::object;
  for (auto& t : table_options_) {
    table_options.insert(t.first, t.second);
  }
  result.insert("TableOptions", table_options);
  result.insert("BlockCacheSizeGb", block_cache_size_gb_);
  result.insert("WriteBufferSizeGb", write_buffer_size_gb_);
  result.insert("NumShardBits", num_shard_bits_);
  result.insert("HighPriPoolRatio", high_pri_pool_ratio_);
  result.insert("StrictCapacityLimit", strict_capacity_limit_);

  return result;
}

bool RocksDbConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* db_options = data.get_ptr("DbOptions");
  if (db_options == nullptr || !db_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_db_options;
  for (auto iter = db_options->keys().begin(); iter != db_options->keys().end(); iter++) {
    if (!db_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_db_options.insert({iter->asString(), db_options->at(*iter).asString()});
  }
  setDbOptions(map_db_options);
  auto* cf_options = data.get_ptr("CfOptions");
  if (cf_options == nullptr || !cf_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_cf_options;
  for (auto iter = cf_options->keys().begin(); iter != cf_options->keys().end(); iter++) {
    if (!cf_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_cf_options.insert({iter->asString(), cf_options->at(*iter).asString()});
  }
  setCfOptions(map_cf_options);
  auto* table_options = data.get_ptr("TableOptions");
  if (table_options == nullptr || !table_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_table_options;
  for (auto iter = table_options->keys().begin(); iter != table_options->keys().end(); iter++) {
    if (!table_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_table_options.insert({iter->asString(), table_options->at(*iter).asString()});
  }
  setTableOptions(map_table_options);
  auto* block_cache_size_gb = data.get_ptr("BlockCacheSizeGb");
  if (block_cache_size_gb == nullptr || !block_cache_size_gb->isInt()) {
    return false;
  }
  setBlockCacheSizeGb(block_cache_size_gb->asInt());
  auto* write_buffer_size_gb = data.get_ptr("WriteBufferSizeGb");
  if (write_buffer_size_gb == nullptr || !write_buffer_size_gb->isInt()) {
    return false;
  }
  setWriteBufferSizeGb(write_buffer_size_gb->asInt());
  auto* num_shard_bits = data.get_ptr("NumShardBits");
  if (num_shard_bits == nullptr || !num_shard_bits->isInt()) {
    return false;
  }
  setNumShardBits(num_shard_bits->asInt());
  auto* high_pri_pool_ratio = data.get_ptr("HighPriPoolRatio");
  if (high_pri_pool_ratio == nullptr || !high_pri_pool_ratio->isDouble()) {
    return false;
  }
  setHighPriPoolRatio(high_pri_pool_ratio->asDouble());
  auto* strict_capacity_limit = data.get_ptr("StrictCapacityLimit");
  if (strict_capacity_limit == nullptr || !strict_capacity_limit->isBool()) {
    return false;
  }
  setStrictCapacityLimit(strict_capacity_limit->asBool());

  return true;
}

void NodeRateLimitEntry::describe(std::ostream& os) const {
  os << "NodeRateLimitEntry{"
     << "BeginHour=" << begin_hour_
     << ", " << "EndHour=" << end_hour_
     << ", " << "RateBytesPerSec=" << rate_bytes_per_sec_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NodeRateLimitEntry& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NodeRateLimitEntry::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("BeginHour", begin_hour_);
  result.insert("EndHour", end_hour_);
  result.insert("RateBytesPerSec", rate_bytes_per_sec_);

  return result;
}

bool NodeRateLimitEntry::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* begin_hour = data.get_ptr("BeginHour");
  if (begin_hour == nullptr || !begin_hour->isInt()) {
    return false;
  }
  setBeginHour(begin_hour->asInt());
  auto* end_hour = data.get_ptr("EndHour");
  if (end_hour == nullptr || !end_hour->isInt()) {
    return false;
  }
  setEndHour(end_hour->asInt());
  auto* rate_bytes_per_sec = data.get_ptr("RateBytesPerSec");
  if (rate_bytes_per_sec == nullptr || !rate_bytes_per_sec->isInt()) {
    return false;
  }
  setRateBytesPerSec(rate_bytes_per_sec->asInt());

  return true;
}

void NodeConfig::describe(std::ostream& os) const {
  os << "NodeConfig{"
     << "BlockCacheSizeGb=" << block_cache_size_gb_
     << ", " << "WriteBufferSizeGb=" << write_buffer_size_gb_
     << ", " << "NumShardBits=" << num_shard_bits_
     << ", " << "HighPriPoolRatio=" << high_pri_pool_ratio_
     << ", " << "StrictCapacityLimit=" << strict_capacity_limit_
     << ", " << "RateLimitStrategy=[";
  for (auto& t : rate_limit_strategy_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NodeConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NodeConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("BlockCacheSizeGb", block_cache_size_gb_);
  result.insert("WriteBufferSizeGb", write_buffer_size_gb_);
  result.insert("NumShardBits", num_shard_bits_);
  result.insert("HighPriPoolRatio", high_pri_pool_ratio_);
  result.insert("StrictCapacityLimit", strict_capacity_limit_);
  folly::dynamic rate_limit_strategy = folly::dynamic::array;
  for (auto& t : rate_limit_strategy_) {
    folly::dynamic vec_rate_limit_strategy = folly::dynamic::object;
    vec_rate_limit_strategy = t.serialize();
    rate_limit_strategy.push_back(vec_rate_limit_strategy);
  }
  result.insert("RateLimitStrategy", rate_limit_strategy);

  return result;
}

bool NodeConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* block_cache_size_gb = data.get_ptr("BlockCacheSizeGb");
  if (block_cache_size_gb == nullptr || !block_cache_size_gb->isInt()) {
    return false;
  }
  setBlockCacheSizeGb(block_cache_size_gb->asInt());
  auto* write_buffer_size_gb = data.get_ptr("WriteBufferSizeGb");
  if (write_buffer_size_gb == nullptr || !write_buffer_size_gb->isInt()) {
    return false;
  }
  setWriteBufferSizeGb(write_buffer_size_gb->asInt());
  auto* num_shard_bits = data.get_ptr("NumShardBits");
  if (num_shard_bits == nullptr || !num_shard_bits->isInt()) {
    return false;
  }
  setNumShardBits(num_shard_bits->asInt());
  auto* high_pri_pool_ratio = data.get_ptr("HighPriPoolRatio");
  if (high_pri_pool_ratio == nullptr || !high_pri_pool_ratio->isDouble()) {
    return false;
  }
  setHighPriPoolRatio(high_pri_pool_ratio->asDouble());
  auto* strict_capacity_limit = data.get_ptr("StrictCapacityLimit");
  if (strict_capacity_limit == nullptr || !strict_capacity_limit->isBool()) {
    return false;
  }
  setStrictCapacityLimit(strict_capacity_limit->asBool());
  auto* rate_limit_strategy = data.get_ptr("RateLimitStrategy");
  if (rate_limit_strategy && rate_limit_strategy->isArray()) {
    std::vector<NodeRateLimitEntry> vec_rate_limit_strategy;
    for (size_t i = 0; i < rate_limit_strategy->size(); i++) {
    if (!rate_limit_strategy->at(i).isObject()) {
      return false;
    }

    NodeRateLimitEntry obj_rate_limit_strategy;
    if (!obj_rate_limit_strategy.deserialize(rate_limit_strategy->at(i))) {
      return false;
    }
    vec_rate_limit_strategy.push_back(obj_rate_limit_strategy);
    }
    setRateLimitStrategy(vec_rate_limit_strategy);
  }

  return true;
}

void TableConfig::describe(std::ostream& os) const {
  os << "TableConfig{"
     << "DbOptions={";
  for (auto& t : db_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "CfOptions={";
  for (auto& t : cf_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "TableOptions={";
  for (auto& t : table_options_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "Version=" << version_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic db_options = folly::dynamic::object;
  for (auto& t : db_options_) {
    db_options.insert(t.first, t.second);
  }
  result.insert("DbOptions", db_options);
  folly::dynamic cf_options = folly::dynamic::object;
  for (auto& t : cf_options_) {
    cf_options.insert(t.first, t.second);
  }
  result.insert("CfOptions", cf_options);
  folly::dynamic table_options = folly::dynamic::object;
  for (auto& t : table_options_) {
    table_options.insert(t.first, t.second);
  }
  result.insert("TableOptions", table_options);
  result.insert("Version", version_);

  return result;
}

bool TableConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* db_options = data.get_ptr("DbOptions");
  if (db_options == nullptr || !db_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_db_options;
  for (auto iter = db_options->keys().begin(); iter != db_options->keys().end(); iter++) {
    if (!db_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_db_options.insert({iter->asString(), db_options->at(*iter).asString()});
  }
  setDbOptions(map_db_options);
  auto* cf_options = data.get_ptr("CfOptions");
  if (cf_options == nullptr || !cf_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_cf_options;
  for (auto iter = cf_options->keys().begin(); iter != cf_options->keys().end(); iter++) {
    if (!cf_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_cf_options.insert({iter->asString(), cf_options->at(*iter).asString()});
  }
  setCfOptions(map_cf_options);
  auto* table_options = data.get_ptr("TableOptions");
  if (table_options == nullptr || !table_options->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_table_options;
  for (auto iter = table_options->keys().begin(); iter != table_options->keys().end(); iter++) {
    if (!table_options->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_table_options.insert({iter->asString(), table_options->at(*iter).asString()});
  }
  setTableOptions(map_table_options);
  auto* version = data.get_ptr("Version");
  if (version == nullptr || !version->isInt()) {
    return false;
  }
  setVersion(version->asInt());

  return true;
}

void ProxySpecificTableConfig::describe(std::ostream& os) const {
  os << "ProxySpecificTableConfig{"
     << "TableReadTimeout=" << table_read_timeout_
     << ", " << "TableWriteTimeout=" << table_write_timeout_
     << ", " << "TableAllowedFlow=" << table_allowed_flow_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ProxySpecificTableConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ProxySpecificTableConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("TableReadTimeout", table_read_timeout_);
  result.insert("TableWriteTimeout", table_write_timeout_);
  result.insert("TableAllowedFlow", table_allowed_flow_);

  return result;
}

bool ProxySpecificTableConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* table_read_timeout = data.get_ptr("TableReadTimeout");
  if (table_read_timeout == nullptr || !table_read_timeout->isInt()) {
    return false;
  }
  setTableReadTimeout(table_read_timeout->asInt());
  auto* table_write_timeout = data.get_ptr("TableWriteTimeout");
  if (table_write_timeout == nullptr || !table_write_timeout->isInt()) {
    return false;
  }
  setTableWriteTimeout(table_write_timeout->asInt());
  auto* table_allowed_flow = data.get_ptr("TableAllowedFlow");
  if (table_allowed_flow == nullptr || !table_allowed_flow->isInt()) {
    return false;
  }
  setTableAllowedFlow(table_allowed_flow->asInt());

  return true;
}

void ProxyDatabaseTableConfig::describe(std::ostream& os) const {
  os << "ProxyDatabaseTableConfig{"
     << "ProxyDbTableConfig={";
  for (auto& t : proxy_db_table_config_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ProxyDatabaseTableConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ProxyDatabaseTableConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic proxy_db_table_config = folly::dynamic::object;
  for (auto& t : proxy_db_table_config_) {
    folly::dynamic map_proxy_db_table_config = folly::dynamic::object;
    map_proxy_db_table_config = t.second.serialize();
    proxy_db_table_config.insert(t.first, map_proxy_db_table_config);
  }
  result.insert("ProxyDbTableConfig", proxy_db_table_config);

  return result;
}

bool ProxyDatabaseTableConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* proxy_db_table_config = data.get_ptr("ProxyDbTableConfig");
  if (proxy_db_table_config == nullptr || !proxy_db_table_config->isObject()) {
    return false;
  }

  std::unordered_map<std::string, ProxySpecificTableConfig> map_proxy_db_table_config;
  for (auto iter = proxy_db_table_config->keys().begin(); iter != proxy_db_table_config->keys().end(); iter++) {
    if (!proxy_db_table_config->at(*iter).isObject()) {
      return false;
    }

    ProxySpecificTableConfig obj_proxy_db_table_config;
    if (!obj_proxy_db_table_config.deserialize(proxy_db_table_config->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_proxy_db_table_config.insert({iter->asString(), obj_proxy_db_table_config});
  }
  setProxyDbTableConfig(map_proxy_db_table_config);

  return true;
}

void ProxyDatabaseTableConfigList::describe(std::ostream& os) const {
  os << "ProxyDatabaseTableConfigList{"
     << "ProxyDbTableConfigList={";
  for (auto& t : proxy_db_table_config_list_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ProxyDatabaseTableConfigList& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ProxyDatabaseTableConfigList::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic proxy_db_table_config_list = folly::dynamic::object;
  for (auto& t : proxy_db_table_config_list_) {
    folly::dynamic map_proxy_db_table_config_list = folly::dynamic::object;
    map_proxy_db_table_config_list = t.second.serialize();
    proxy_db_table_config_list.insert(t.first, map_proxy_db_table_config_list);
  }
  result.insert("ProxyDbTableConfigList", proxy_db_table_config_list);

  return result;
}

bool ProxyDatabaseTableConfigList::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* proxy_db_table_config_list = data.get_ptr("ProxyDbTableConfigList");
  if (proxy_db_table_config_list == nullptr || !proxy_db_table_config_list->isObject()) {
    return false;
  }

  std::unordered_map<std::string, ProxyDatabaseTableConfig> map_proxy_db_table_config_list;
  for (auto iter = proxy_db_table_config_list->keys().begin(); iter != proxy_db_table_config_list->keys().end(); iter++) {
    if (!proxy_db_table_config_list->at(*iter).isObject()) {
      return false;
    }

    ProxyDatabaseTableConfig obj_proxy_db_table_config_list;
    if (!obj_proxy_db_table_config_list.deserialize(proxy_db_table_config_list->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_proxy_db_table_config_list.insert({iter->asString(), obj_proxy_db_table_config_list});
  }
  setProxyDbTableConfigList(map_proxy_db_table_config_list);

  return true;
}

void ProxyTableConfig::describe(std::ostream& os) const {
  os << "ProxyTableConfig{"
     << "LaserClientReadTimeout=" << laser_client_read_timeout_
     << ", " << "LaserClientWriteTimeout=" << laser_client_write_timeout_
     << ", " << "LaserClientAllowedFlow=" << laser_client_allowed_flow_
     << ", " << "ProxyTableConfig=" << proxy_table_config_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ProxyTableConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ProxyTableConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("LaserClientReadTimeout", laser_client_read_timeout_);
  result.insert("LaserClientWriteTimeout", laser_client_write_timeout_);
  result.insert("LaserClientAllowedFlow", laser_client_allowed_flow_);
  folly::dynamic proxy_table_config = folly::dynamic::object;
  proxy_table_config = proxy_table_config_.serialize();
  result.insert("ProxyTableConfig", proxy_table_config);

  return result;
}

bool ProxyTableConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* laser_client_read_timeout = data.get_ptr("LaserClientReadTimeout");
  if (laser_client_read_timeout == nullptr || !laser_client_read_timeout->isInt()) {
    return false;
  }
  setLaserClientReadTimeout(laser_client_read_timeout->asInt());
  auto* laser_client_write_timeout = data.get_ptr("LaserClientWriteTimeout");
  if (laser_client_write_timeout == nullptr || !laser_client_write_timeout->isInt()) {
    return false;
  }
  setLaserClientWriteTimeout(laser_client_write_timeout->asInt());
  auto* laser_client_allowed_flow = data.get_ptr("LaserClientAllowedFlow");
  if (laser_client_allowed_flow == nullptr || !laser_client_allowed_flow->isInt()) {
    return false;
  }
  setLaserClientAllowedFlow(laser_client_allowed_flow->asInt());
  auto* proxy_table_config = data.get_ptr("ProxyTableConfig");
  if (proxy_table_config == nullptr) {
    return false;
  }

  ProxyDatabaseTableConfigList itemproxy_table_config;
  if (!itemproxy_table_config.deserialize(*proxy_table_config)) {
    return false;
  }
  setProxyTableConfig(itemproxy_table_config);

  return true;
}

void NodeConfigList::describe(std::ostream& os) const {
  os << "NodeConfigList{"
     << "NodeConfigList={";
  for (auto& t : node_config_list_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NodeConfigList& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NodeConfigList::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic node_config_list = folly::dynamic::object;
  for (auto& t : node_config_list_) {
    folly::dynamic map_node_config_list = folly::dynamic::object;
    map_node_config_list = t.second.serialize();
    node_config_list.insert(t.first, map_node_config_list);
  }
  result.insert("NodeConfigList", node_config_list);

  return result;
}

bool NodeConfigList::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* node_config_list = data.get_ptr("NodeConfigList");
  if (node_config_list == nullptr || !node_config_list->isObject()) {
    return false;
  }

  std::unordered_map<std::string, NodeConfig> map_node_config_list;
  for (auto iter = node_config_list->keys().begin(); iter != node_config_list->keys().end(); iter++) {
    if (!node_config_list->at(*iter).isObject()) {
      return false;
    }

    NodeConfig obj_node_config_list;
    if (!obj_node_config_list.deserialize(node_config_list->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_node_config_list.insert({iter->asString(), obj_node_config_list});
  }
  setNodeConfigList(map_node_config_list);

  return true;
}

void TableConfigList::describe(std::ostream& os) const {
  os << "TableConfigList{"
     << "TableConfigList={";
  for (auto& t : table_config_list_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableConfigList& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableConfigList::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic table_config_list = folly::dynamic::object;
  for (auto& t : table_config_list_) {
    folly::dynamic map_table_config_list = folly::dynamic::object;
    map_table_config_list = t.second.serialize();
    table_config_list.insert(t.first, map_table_config_list);
  }
  result.insert("TableConfigList", table_config_list);

  return result;
}

bool TableConfigList::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* table_config_list = data.get_ptr("TableConfigList");
  if (table_config_list == nullptr || !table_config_list->isObject()) {
    return false;
  }

  std::unordered_map<std::string, TableConfig> map_table_config_list;
  for (auto iter = table_config_list->keys().begin(); iter != table_config_list->keys().end(); iter++) {
    if (!table_config_list->at(*iter).isObject()) {
      return false;
    }

    TableConfig obj_table_config_list;
    if (!obj_table_config_list.deserialize(table_config_list->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_table_config_list.insert({iter->asString(), obj_table_config_list});
  }
  setTableConfigList(map_table_config_list);

  return true;
}

void RocksdbNodeConfigs::describe(std::ostream& os) const {
  os << "RocksdbNodeConfigs{"
     << "RocksdbNodeConfigs={";
  for (auto& t : rocksdb_node_configs_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const RocksdbNodeConfigs& value) {
  value.describe(os);
  return os;
}

const folly::dynamic RocksdbNodeConfigs::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic rocksdb_node_configs = folly::dynamic::object;
  for (auto& t : rocksdb_node_configs_) {
    rocksdb_node_configs.insert(t.first, t.second);
  }
  result.insert("RocksdbNodeConfigs", rocksdb_node_configs);

  return result;
}

bool RocksdbNodeConfigs::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* rocksdb_node_configs = data.get_ptr("RocksdbNodeConfigs");
  if (rocksdb_node_configs == nullptr || !rocksdb_node_configs->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_rocksdb_node_configs;
  for (auto iter = rocksdb_node_configs->keys().begin(); iter != rocksdb_node_configs->keys().end(); iter++) {
    if (!rocksdb_node_configs->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_rocksdb_node_configs.insert({iter->asString(), rocksdb_node_configs->at(*iter).asString()});
  }
  setRocksdbNodeConfigs(map_rocksdb_node_configs);

  return true;
}

std::ostream& operator<<(std::ostream& os, const TrafficRestrictionType& value) {
  os << static_cast<int>(value);
  return os;
}

folly::dynamic serializeTrafficRestrictionType(const TrafficRestrictionType& value) {
  folly::dynamic result = static_cast<int>(value);
  return result;
}

bool deserializeTrafficRestrictionType(const folly::dynamic& data, TrafficRestrictionType* result) {
  if (!data.isInt()) {
    return false;
  }

  int value = data.asInt();
  switch(value) {
    case 0:
      *result = TrafficRestrictionType::KPS;
      break;
    case 1:
      *result = TrafficRestrictionType::QPS;
      break;
    default:
      return false;
  }

  return true;
}

void TrafficRestrictionLimitItem::describe(std::ostream& os) const {
  os << "TrafficRestrictionLimitItem{"
     << "Type=" << type_
     << ", " << "Limit=" << limit_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TrafficRestrictionLimitItem& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TrafficRestrictionLimitItem::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic type = serializeTrafficRestrictionType(type_);
  result.insert("Type", type);
  result.insert("Limit", limit_);

  return result;
}

bool TrafficRestrictionLimitItem::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* type = data.get_ptr("Type");
  if (type == nullptr) {
    return false;
  }

  TrafficRestrictionType itemtype;
  if (!deserializeTrafficRestrictionType(*type, &itemtype)) {
    return false;
  }
  setType(itemtype);
  auto* limit = data.get_ptr("Limit");
  if (limit == nullptr || !limit->isInt()) {
    return false;
  }
  setLimit(limit->asInt());

  return true;
}

void TableTrafficRestrictionConfig::describe(std::ostream& os) const {
  os << "TableTrafficRestrictionConfig{"
     << "TableName='" << table_name_ << "'"
     << ", " << "DenyAll=" << deny_all_
     << ", " << "SingleOperationLimits={";
  for (auto& t : single_operation_limits_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", " << "MultipleOperationLimits={";
  for (auto& t : multiple_operation_limits_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableTrafficRestrictionConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableTrafficRestrictionConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("TableName", table_name_);
  result.insert("DenyAll", deny_all_);
  folly::dynamic single_operation_limits = folly::dynamic::object;
  for (auto& t : single_operation_limits_) {
    single_operation_limits.insert(t.first, t.second);
  }
  result.insert("SingleOperationLimits", single_operation_limits);
  folly::dynamic multiple_operation_limits = folly::dynamic::object;
  for (auto& t : multiple_operation_limits_) {
    folly::dynamic map_multiple_operation_limits = folly::dynamic::object;
    map_multiple_operation_limits = t.second.serialize();
    multiple_operation_limits.insert(t.first, map_multiple_operation_limits);
  }
  result.insert("MultipleOperationLimits", multiple_operation_limits);

  return result;
}

bool TableTrafficRestrictionConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* table_name = data.get_ptr("TableName");
  if (table_name == nullptr || !table_name->isString()) {
    return false;
  }
  setTableName(table_name->asString());
  auto* deny_all = data.get_ptr("DenyAll");
  if (deny_all == nullptr || !deny_all->isBool()) {
    return false;
  }
  setDenyAll(deny_all->asBool());
  auto* single_operation_limits = data.get_ptr("SingleOperationLimits");
  if (single_operation_limits == nullptr || !single_operation_limits->isObject()) {
    return false;
  }

  std::unordered_map<std::string, uint32_t> map_single_operation_limits;
  for (auto iter = single_operation_limits->keys().begin(); iter != single_operation_limits->keys().end(); iter++) {
    if (!single_operation_limits->at(*iter).isInt()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_single_operation_limits.insert({iter->asString(), single_operation_limits->at(*iter).asInt()});
  }
  setSingleOperationLimits(map_single_operation_limits);
  auto* multiple_operation_limits = data.get_ptr("MultipleOperationLimits");
  if (multiple_operation_limits == nullptr || !multiple_operation_limits->isObject()) {
    return false;
  }

  std::unordered_map<std::string, TrafficRestrictionLimitItem> map_multiple_operation_limits;
  for (auto iter = multiple_operation_limits->keys().begin(); iter != multiple_operation_limits->keys().end(); iter++) {
    if (!multiple_operation_limits->at(*iter).isObject()) {
      return false;
    }

    TrafficRestrictionLimitItem obj_multiple_operation_limits;
    if (!obj_multiple_operation_limits.deserialize(multiple_operation_limits->at(*iter))) {
      return false;
    }
    if (!iter->isString()) {
      return false;
    }
    map_multiple_operation_limits.insert({iter->asString(), obj_multiple_operation_limits});
  }
  setMultipleOperationLimits(map_multiple_operation_limits);

  return true;
}

void DatabaseTrafficRestrictionConfig::describe(std::ostream& os) const {
  os << "DatabaseTrafficRestrictionConfig{"
     << "DatabaseName='" << database_name_ << "'"
     << ", " << "Tables=[";
  for (auto& t : tables_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const DatabaseTrafficRestrictionConfig& value) {
  value.describe(os);
  return os;
}

const folly::dynamic DatabaseTrafficRestrictionConfig::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("DatabaseName", database_name_);
  folly::dynamic tables = folly::dynamic::array;
  for (auto& t : tables_) {
    folly::dynamic vec_tables = folly::dynamic::object;
    vec_tables = t.serialize();
    tables.push_back(vec_tables);
  }
  result.insert("Tables", tables);

  return result;
}

bool DatabaseTrafficRestrictionConfig::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* database_name = data.get_ptr("DatabaseName");
  if (database_name == nullptr || !database_name->isString()) {
    return false;
  }
  setDatabaseName(database_name->asString());
  auto* tables = data.get_ptr("Tables");
  if (tables == nullptr || !tables->isArray()) {
    return false;
  }

  std::vector<TableTrafficRestrictionConfig> vec_tables;
  for (size_t i = 0; i < tables->size(); i++) {
    if (!tables->at(i).isObject()) {
      return false;
    }

    TableTrafficRestrictionConfig obj_tables;
    if (!obj_tables.deserialize(tables->at(i))) {
      return false;
    }
    vec_tables.push_back(obj_tables);
  }
  setTables(vec_tables);

  return true;
}

std::ostream& operator<<(std::ostream& os, const TableLoaderType& value) {
  switch(value) {
    case TableLoaderType::BASE:
      os << "base";
      break;
    case TableLoaderType::DELTA:
      os << "delta";
      break;
    case TableLoaderType::STREAM:
      os << "stream";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeTableLoaderType(const TableLoaderType& value) {
  folly::dynamic result = toStringTableLoaderType(value);
  return result;
}

bool deserializeTableLoaderType(const folly::dynamic& data, TableLoaderType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToTableLoaderType(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<TableLoaderType> stringToTableLoaderType(const std::string& name) {
  if (name == "base") {
    return TableLoaderType::BASE;
  }
  if (name == "delta") {
    return TableLoaderType::DELTA;
  }
  if (name == "stream") {
    return TableLoaderType::STREAM;
  }
     
  return folly::none;
}

const std::string toStringTableLoaderType(const TableLoaderType& value) {
  std::string result;
  switch(value) {
    case TableLoaderType::BASE:
      result = "base";
      break;
    case TableLoaderType::DELTA:
      result = "delta";
      break;
    case TableLoaderType::STREAM:
      result = "stream";
      break;
    default:
      result = "unknow";
  }

  return result;
}


std::ostream& operator<<(std::ostream& os, const TableLoaderSourceType& value) {
  switch(value) {
    case TableLoaderSourceType::HDFS:
      os << "hdfs";
      break;
    case TableLoaderSourceType::KAFKA:
      os << "kafka";
      break;
    case TableLoaderSourceType::LOCAL:
      os << "local";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeTableLoaderSourceType(const TableLoaderSourceType& value) {
  folly::dynamic result = toStringTableLoaderSourceType(value);
  return result;
}

bool deserializeTableLoaderSourceType(const folly::dynamic& data, TableLoaderSourceType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToTableLoaderSourceType(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<TableLoaderSourceType> stringToTableLoaderSourceType(const std::string& name) {
  if (name == "hdfs") {
    return TableLoaderSourceType::HDFS;
  }
  if (name == "kafka") {
    return TableLoaderSourceType::KAFKA;
  }
  if (name == "local") {
    return TableLoaderSourceType::LOCAL;
  }
     
  return folly::none;
}

const std::string toStringTableLoaderSourceType(const TableLoaderSourceType& value) {
  std::string result;
  switch(value) {
    case TableLoaderSourceType::HDFS:
      result = "hdfs";
      break;
    case TableLoaderSourceType::KAFKA:
      result = "kafka";
      break;
    case TableLoaderSourceType::LOCAL:
      result = "local";
      break;
    default:
      result = "unknow";
  }

  return result;
}


void TableDataLoaderSchema::describe(std::ostream& os) const {
  os << "TableDataLoaderSchema{"
     << "Type=" << type_
     << ", " << "SourceType=" << source_type_
     << ", " << "Params={";
  for (auto& t : params_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableDataLoaderSchema& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableDataLoaderSchema::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic type = serializeTableLoaderType(type_);
  result.insert("Type", type);
  folly::dynamic source_type = serializeTableLoaderSourceType(source_type_);
  result.insert("SourceType", source_type);
  folly::dynamic params = folly::dynamic::object;
  for (auto& t : params_) {
    params.insert(t.first, t.second);
  }
  result.insert("Params", params);

  return result;
}

bool TableDataLoaderSchema::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* type = data.get_ptr("Type");
  if (type == nullptr) {
    return false;
  }

  TableLoaderType itemtype;
  if (!deserializeTableLoaderType(*type, &itemtype)) {
    return false;
  }
  setType(itemtype);
  auto* source_type = data.get_ptr("SourceType");
  if (source_type == nullptr) {
    return false;
  }

  TableLoaderSourceType itemsource_type;
  if (!deserializeTableLoaderSourceType(*source_type, &itemsource_type)) {
    return false;
  }
  setSourceType(itemsource_type);
  auto* params = data.get_ptr("Params");
  if (params == nullptr || !params->isObject()) {
    return false;
  }

  std::unordered_map<std::string, std::string> map_params;
  for (auto iter = params->keys().begin(); iter != params->keys().end(); iter++) {
    if (!params->at(*iter).isString()) {
      return false;
    }
  
    if (!iter->isString()) {
      return false;
    }
    map_params.insert({iter->asString(), params->at(*iter).asString()});
  }
  setParams(map_params);

  return true;
}

std::ostream& operator<<(std::ostream& os, const TableEngineType& value) {
  switch(value) {
    case TableEngineType::ROCKSDB:
      os << "rocksdb";
      break;
    case TableEngineType::UNORDERED_MAP:
      os << "unordered_map";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeTableEngineType(const TableEngineType& value) {
  folly::dynamic result = toStringTableEngineType(value);
  return result;
}

bool deserializeTableEngineType(const folly::dynamic& data, TableEngineType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToTableEngineType(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<TableEngineType> stringToTableEngineType(const std::string& name) {
  if (name == "rocksdb") {
    return TableEngineType::ROCKSDB;
  }
  if (name == "unordered_map") {
    return TableEngineType::UNORDERED_MAP;
  }
     
  return folly::none;
}

const std::string toStringTableEngineType(const TableEngineType& value) {
  std::string result;
  switch(value) {
    case TableEngineType::ROCKSDB:
      result = "rocksdb";
      break;
    case TableEngineType::UNORDERED_MAP:
      result = "unordered_map";
      break;
    default:
      result = "unknow";
  }

  return result;
}


void TableSchema::describe(std::ostream& os) const {
  os << "TableSchema{"
     << "DatabaseName='" << database_name_ << "'"
     << ", " << "TableName='" << table_name_ << "'"
     << ", " << "PartitionNumber=" << partition_number_
     << ", " << "Ttl=" << ttl_
     << ", " << "ConfigName='" << config_name_ << "'"
     << ", " << "BindEdgeNodes=[";
  for (auto& t : bind_edge_nodes_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "EdgeFlowRatio=" << edge_flow_ratio_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableSchema& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableSchema::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("DatabaseName", database_name_);
  result.insert("TableName", table_name_);
  result.insert("PartitionNumber", partition_number_);
  result.insert("Ttl", ttl_);
  result.insert("ConfigName", config_name_);
  folly::dynamic bind_edge_nodes = folly::dynamic::array;
  for (auto& t : bind_edge_nodes_) {
    bind_edge_nodes.push_back(t);
  }
  result.insert("BindEdgeNodes", bind_edge_nodes);
  result.insert("EdgeFlowRatio", edge_flow_ratio_);

  return result;
}

bool TableSchema::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* database_name = data.get_ptr("DatabaseName");
  if (database_name && database_name->isString()) {
    setDatabaseName(database_name->asString());
  }
  auto* table_name = data.get_ptr("TableName");
  if (table_name == nullptr || !table_name->isString()) {
    return false;
  }
  setTableName(table_name->asString());
  auto* partition_number = data.get_ptr("PartitionNumber");
  if (partition_number == nullptr || !partition_number->isInt()) {
    return false;
  }
  setPartitionNumber(partition_number->asInt());
  auto* ttl = data.get_ptr("Ttl");
  if (ttl && ttl->isInt()) {
    setTtl(ttl->asInt());
  }
  auto* config_name = data.get_ptr("ConfigName");
  if (config_name && config_name->isString()) {
    setConfigName(config_name->asString());
  }
  auto* bind_edge_nodes = data.get_ptr("BindEdgeNodes");
  if (bind_edge_nodes && bind_edge_nodes->isArray()) {
    std::vector<std::string> vec_bind_edge_nodes;
    for (size_t i = 0; i < bind_edge_nodes->size(); i++) {
    if (!bind_edge_nodes->at(i).isString()) {
      return false;
    }
    vec_bind_edge_nodes.push_back(bind_edge_nodes->at(i).asString());
    }
    setBindEdgeNodes(vec_bind_edge_nodes);
  }
  auto* edge_flow_ratio = data.get_ptr("EdgeFlowRatio");
  if (edge_flow_ratio && edge_flow_ratio->isInt()) {
    setEdgeFlowRatio(edge_flow_ratio->asInt());
  }

  return true;
}

void DatabaseSchema::describe(std::ostream& os) const {
  os << "DatabaseSchema{"
     << "DatabaseName='" << database_name_ << "'"
     << ", " << "Tables=[";
  for (auto& t : tables_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const DatabaseSchema& value) {
  value.describe(os);
  return os;
}

const folly::dynamic DatabaseSchema::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("DatabaseName", database_name_);
  folly::dynamic tables = folly::dynamic::array;
  for (auto& t : tables_) {
    folly::dynamic vec_tables = folly::dynamic::object;
    vec_tables = t.serialize();
    tables.push_back(vec_tables);
  }
  result.insert("Tables", tables);

  return result;
}

bool DatabaseSchema::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* database_name = data.get_ptr("DatabaseName");
  if (database_name == nullptr || !database_name->isString()) {
    return false;
  }
  setDatabaseName(database_name->asString());
  auto* tables = data.get_ptr("Tables");
  if (tables == nullptr || !tables->isArray()) {
    return false;
  }

  std::vector<TableSchema> vec_tables;
  for (size_t i = 0; i < tables->size(); i++) {
    if (!tables->at(i).isObject()) {
      return false;
    }

    TableSchema obj_tables;
    if (!obj_tables.deserialize(tables->at(i))) {
      return false;
    }
    vec_tables.push_back(obj_tables);
  }
  setTables(vec_tables);

  return true;
}

void NodeShardList::describe(std::ostream& os) const {
  os << "NodeShardList{"
     << "NodeId=" << node_id_
     << ", " << "LeaderShardList=[";
  for (auto& t : leader_shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "FollowerShardList=[";
  for (auto& t : follower_shard_list_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "IsEdgeNode=" << is_edge_node_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const NodeShardList& value) {
  value.describe(os);
  return os;
}

const folly::dynamic NodeShardList::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("NodeId", node_id_);
  folly::dynamic leader_shard_list = folly::dynamic::array;
  for (auto& t : leader_shard_list_) {
    leader_shard_list.push_back(t);
  }
  result.insert("LeaderShardList", leader_shard_list);
  folly::dynamic follower_shard_list = folly::dynamic::array;
  for (auto& t : follower_shard_list_) {
    follower_shard_list.push_back(t);
  }
  result.insert("FollowerShardList", follower_shard_list);
  result.insert("IsEdgeNode", is_edge_node_);

  return result;
}

bool NodeShardList::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* node_id = data.get_ptr("NodeId");
  if (node_id == nullptr || !node_id->isInt()) {
    return false;
  }
  setNodeId(node_id->asInt());
  auto* leader_shard_list = data.get_ptr("LeaderShardList");
  if (leader_shard_list == nullptr || !leader_shard_list->isArray()) {
    return false;
  }

  std::vector<uint32_t> vec_leader_shard_list;
  for (size_t i = 0; i < leader_shard_list->size(); i++) {
    if (!leader_shard_list->at(i).isInt()) {
      return false;
    }
    vec_leader_shard_list.push_back(leader_shard_list->at(i).asInt());
  }
  setLeaderShardList(vec_leader_shard_list);
  auto* follower_shard_list = data.get_ptr("FollowerShardList");
  if (follower_shard_list == nullptr || !follower_shard_list->isArray()) {
    return false;
  }

  std::vector<uint32_t> vec_follower_shard_list;
  for (size_t i = 0; i < follower_shard_list->size(); i++) {
    if (!follower_shard_list->at(i).isInt()) {
      return false;
    }
    vec_follower_shard_list.push_back(follower_shard_list->at(i).asInt());
  }
  setFollowerShardList(vec_follower_shard_list);
  auto* is_edge_node = data.get_ptr("IsEdgeNode");
  if (is_edge_node && is_edge_node->isBool()) {
    setIsEdgeNode(is_edge_node->asBool());
  }

  return true;
}

void ClusterGroup::describe(std::ostream& os) const {
  os << "ClusterGroup{"
     << "GroupName='" << group_name_ << "'"
     << ", " << "Nodes=[";
  for (auto& t : nodes_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ClusterGroup& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ClusterGroup::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("GroupName", group_name_);
  folly::dynamic nodes = folly::dynamic::array;
  for (auto& t : nodes_) {
    folly::dynamic vec_nodes = folly::dynamic::object;
    vec_nodes = t.serialize();
    nodes.push_back(vec_nodes);
  }
  result.insert("Nodes", nodes);

  return result;
}

bool ClusterGroup::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* group_name = data.get_ptr("GroupName");
  if (group_name == nullptr || !group_name->isString()) {
    return false;
  }
  setGroupName(group_name->asString());
  auto* nodes = data.get_ptr("Nodes");
  if (nodes == nullptr || !nodes->isArray()) {
    return false;
  }

  std::vector<NodeShardList> vec_nodes;
  for (size_t i = 0; i < nodes->size(); i++) {
    if (!nodes->at(i).isObject()) {
      return false;
    }

    NodeShardList obj_nodes;
    if (!obj_nodes.deserialize(nodes->at(i))) {
      return false;
    }
    vec_nodes.push_back(obj_nodes);
  }
  setNodes(vec_nodes);

  return true;
}

void ClusterInfo::describe(std::ostream& os) const {
  os << "ClusterInfo{"
     << "ClusterName='" << cluster_name_ << "'"
     << ", " << "ShardNumber=" << shard_number_
     << ", " << "Groups=[";
  for (auto& t : groups_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ClusterInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ClusterInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("ClusterName", cluster_name_);
  result.insert("ShardNumber", shard_number_);
  folly::dynamic groups = folly::dynamic::array;
  for (auto& t : groups_) {
    folly::dynamic vec_groups = folly::dynamic::object;
    vec_groups = t.serialize();
    groups.push_back(vec_groups);
  }
  result.insert("Groups", groups);

  return result;
}

bool ClusterInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* cluster_name = data.get_ptr("ClusterName");
  if (cluster_name == nullptr || !cluster_name->isString()) {
    return false;
  }
  setClusterName(cluster_name->asString());
  auto* shard_number = data.get_ptr("ShardNumber");
  if (shard_number == nullptr || !shard_number->isInt()) {
    return false;
  }
  setShardNumber(shard_number->asInt());
  auto* groups = data.get_ptr("Groups");
  if (groups == nullptr || !groups->isArray()) {
    return false;
  }

  std::vector<ClusterGroup> vec_groups;
  for (size_t i = 0; i < groups->size(); i++) {
    if (!groups->at(i).isObject()) {
      return false;
    }

    ClusterGroup obj_groups;
    if (!obj_groups.deserialize(groups->at(i))) {
      return false;
    }
    vec_groups.push_back(obj_groups);
  }
  setGroups(vec_groups);

  return true;
}

std::ostream& operator<<(std::ostream& os, const DBRole& value) {
  switch(value) {
    case DBRole::LEADER:
      os << "leader";
      break;
    case DBRole::FOLLOWER:
      os << "follower";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeDBRole(const DBRole& value) {
  folly::dynamic result = toStringDBRole(value);
  return result;
}

bool deserializeDBRole(const folly::dynamic& data, DBRole* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToDBRole(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<DBRole> stringToDBRole(const std::string& name) {
  if (name == "leader") {
    return DBRole::LEADER;
  }
  if (name == "follower") {
    return DBRole::FOLLOWER;
  }
     
  return folly::none;
}

const std::string toStringDBRole(const DBRole& value) {
  std::string result;
  switch(value) {
    case DBRole::LEADER:
      result = "leader";
      break;
    case DBRole::FOLLOWER:
      result = "follower";
      break;
    default:
      result = "unknow";
  }

  return result;
}


void ReplicationDbMetaInfo::describe(std::ostream& os) const {
  os << "ReplicationDbMetaInfo{"
     << "SeqNo=" << seq_no_
     << ", " << "ReplicateLag=" << replicate_lag_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ReplicationDbMetaInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ReplicationDbMetaInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("SeqNo", seq_no_);
  result.insert("ReplicateLag", replicate_lag_);

  return result;
}

bool ReplicationDbMetaInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* seq_no = data.get_ptr("SeqNo");
  if (seq_no == nullptr || !seq_no->isInt()) {
    return false;
  }
  setSeqNo(seq_no->asInt());
  auto* replicate_lag = data.get_ptr("ReplicateLag");
  if (replicate_lag == nullptr || !replicate_lag->isInt()) {
    return false;
  }
  setReplicateLag(replicate_lag->asInt());

  return true;
}

void PartitionMetaInfo::describe(std::ostream& os) const {
  os << "PartitionMetaInfo{"
     << "Role=" << role_
     << ", " << "PartitionId=" << partition_id_
     << ", " << "Hash=" << hash_
     << ", " << "Size=" << size_
     << ", " << "ReadKps=" << read_kps_
     << ", " << "WriteKps=" << write_kps_
     << ", " << "ReadBytes=" << read_bytes_
     << ", " << "WriteBytes=" << write_bytes_
     << ", " << "DatabaseName='" << database_name_ << "'"
     << ", " << "TableName='" << table_name_ << "'"
     << ", " << "BaseVersion='" << base_version_ << "'"
     << ", " << "DeltaVersions=[";
  for (auto& t : delta_versions_) {
    os << t << ",";
  }
  os << "]"
     << ", " << "DbInfo=" << db_info_
     << "}";
}

std::ostream& operator<<(std::ostream& os, const PartitionMetaInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic PartitionMetaInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic role = serializeDBRole(role_);
  result.insert("Role", role);
  result.insert("PartitionId", partition_id_);
  result.insert("Hash", hash_);
  result.insert("Size", size_);
  result.insert("ReadKps", read_kps_);
  result.insert("WriteKps", write_kps_);
  result.insert("ReadBytes", read_bytes_);
  result.insert("WriteBytes", write_bytes_);
  result.insert("DatabaseName", database_name_);
  result.insert("TableName", table_name_);
  result.insert("BaseVersion", base_version_);
  folly::dynamic delta_versions = folly::dynamic::array;
  for (auto& t : delta_versions_) {
    delta_versions.push_back(t);
  }
  result.insert("DeltaVersions", delta_versions);
  folly::dynamic db_info = folly::dynamic::object;
  db_info = db_info_.serialize();
  result.insert("DbInfo", db_info);

  return result;
}

bool PartitionMetaInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* role = data.get_ptr("Role");
  if (role == nullptr) {
    return false;
  }

  DBRole itemrole;
  if (!deserializeDBRole(*role, &itemrole)) {
    return false;
  }
  setRole(itemrole);
  auto* partition_id = data.get_ptr("PartitionId");
  if (partition_id == nullptr || !partition_id->isInt()) {
    return false;
  }
  setPartitionId(partition_id->asInt());
  auto* hash = data.get_ptr("Hash");
  if (hash == nullptr || !hash->isInt()) {
    return false;
  }
  setHash(hash->asInt());
  auto* size = data.get_ptr("Size");
  if (size == nullptr || !size->isInt()) {
    return false;
  }
  setSize(size->asInt());
  auto* read_kps = data.get_ptr("ReadKps");
  if (read_kps == nullptr || !read_kps->isInt()) {
    return false;
  }
  setReadKps(read_kps->asInt());
  auto* write_kps = data.get_ptr("WriteKps");
  if (write_kps == nullptr || !write_kps->isInt()) {
    return false;
  }
  setWriteKps(write_kps->asInt());
  auto* read_bytes = data.get_ptr("ReadBytes");
  if (read_bytes == nullptr || !read_bytes->isInt()) {
    return false;
  }
  setReadBytes(read_bytes->asInt());
  auto* write_bytes = data.get_ptr("WriteBytes");
  if (write_bytes == nullptr || !write_bytes->isInt()) {
    return false;
  }
  setWriteBytes(write_bytes->asInt());
  auto* database_name = data.get_ptr("DatabaseName");
  if (database_name == nullptr || !database_name->isString()) {
    return false;
  }
  setDatabaseName(database_name->asString());
  auto* table_name = data.get_ptr("TableName");
  if (table_name == nullptr || !table_name->isString()) {
    return false;
  }
  setTableName(table_name->asString());
  auto* base_version = data.get_ptr("BaseVersion");
  if (base_version == nullptr || !base_version->isString()) {
    return false;
  }
  setBaseVersion(base_version->asString());
  auto* delta_versions = data.get_ptr("DeltaVersions");
  if (delta_versions == nullptr || !delta_versions->isArray()) {
    return false;
  }

  std::vector<std::string> vec_delta_versions;
  for (size_t i = 0; i < delta_versions->size(); i++) {
    if (!delta_versions->at(i).isString()) {
      return false;
    }
    vec_delta_versions.push_back(delta_versions->at(i).asString());
  }
  setDeltaVersions(vec_delta_versions);
  auto* db_info = data.get_ptr("DbInfo");
  if (db_info == nullptr) {
    return false;
  }

  ReplicationDbMetaInfo itemdb_info;
  if (!itemdb_info.deserialize(*db_info)) {
    return false;
  }
  setDbInfo(itemdb_info);

  return true;
}

void TableMetaInfo::describe(std::ostream& os) const {
  os << "TableMetaInfo{"
     << "GroupName='" << group_name_ << "'"
     << ", " << "NodeId=" << node_id_
     << ", " << "Partitions=[";
  for (auto& t : partitions_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const TableMetaInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic TableMetaInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("GroupName", group_name_);
  result.insert("NodeId", node_id_);
  folly::dynamic partitions = folly::dynamic::array;
  for (auto& t : partitions_) {
    folly::dynamic vec_partitions = folly::dynamic::object;
    vec_partitions = t.serialize();
    partitions.push_back(vec_partitions);
  }
  result.insert("Partitions", partitions);

  return result;
}

bool TableMetaInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* group_name = data.get_ptr("GroupName");
  if (group_name == nullptr || !group_name->isString()) {
    return false;
  }
  setGroupName(group_name->asString());
  auto* node_id = data.get_ptr("NodeId");
  if (node_id == nullptr || !node_id->isInt()) {
    return false;
  }
  setNodeId(node_id->asInt());
  auto* partitions = data.get_ptr("Partitions");
  if (partitions == nullptr || !partitions->isArray()) {
    return false;
  }

  std::vector<PartitionMetaInfo> vec_partitions;
  for (size_t i = 0; i < partitions->size(); i++) {
    if (!partitions->at(i).isObject()) {
      return false;
    }

    PartitionMetaInfo obj_partitions;
    if (!obj_partitions.deserialize(partitions->at(i))) {
      return false;
    }
    vec_partitions.push_back(obj_partitions);
  }
  setPartitions(vec_partitions);

  return true;
}

std::ostream& operator<<(std::ostream& os, const ShardServiceStatus& value) {
  switch(value) {
    case ShardServiceStatus::AVAILABLE:
      os << "available";
      break;
    case ShardServiceStatus::UNAVAILABLE:
      os << "unavailable";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeShardServiceStatus(const ShardServiceStatus& value) {
  folly::dynamic result = toStringShardServiceStatus(value);
  return result;
}

bool deserializeShardServiceStatus(const folly::dynamic& data, ShardServiceStatus* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToShardServiceStatus(value);
  if (enum_obj) {
    *result = *enum_obj;    
    return true;
  }

  return false;
}

folly::Optional<ShardServiceStatus> stringToShardServiceStatus(const std::string& name) {
  if (name == "available") {
    return ShardServiceStatus::AVAILABLE;
  }
  if (name == "unavailable") {
    return ShardServiceStatus::UNAVAILABLE;
  }
     
  return folly::none;
}

const std::string toStringShardServiceStatus(const ShardServiceStatus& value) {
  std::string result;
  switch(value) {
    case ShardServiceStatus::AVAILABLE:
      result = "available";
      break;
    case ShardServiceStatus::UNAVAILABLE:
      result = "unavailable";
      break;
    default:
      result = "unknow";
  }

  return result;
}


void ShardMetaInfo::describe(std::ostream& os) const {
  os << "ShardMetaInfo{"
     << "ShardId=" << shard_id_
     << ", " << "Role=" << role_
     << ", " << "Status=" << status_
     << ", " << "Partitions=[";
  for (auto& t : partitions_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const ShardMetaInfo& value) {
  value.describe(os);
  return os;
}

const folly::dynamic ShardMetaInfo::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  result.insert("ShardId", shard_id_);
  folly::dynamic role = serializeDBRole(role_);
  result.insert("Role", role);
  folly::dynamic status = serializeShardServiceStatus(status_);
  result.insert("Status", status);
  folly::dynamic partitions = folly::dynamic::array;
  for (auto& t : partitions_) {
    folly::dynamic vec_partitions = folly::dynamic::object;
    vec_partitions = t.serialize();
    partitions.push_back(vec_partitions);
  }
  result.insert("Partitions", partitions);

  return result;
}

bool ShardMetaInfo::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* shard_id = data.get_ptr("ShardId");
  if (shard_id == nullptr || !shard_id->isInt()) {
    return false;
  }
  setShardId(shard_id->asInt());
  auto* role = data.get_ptr("Role");
  if (role == nullptr) {
    return false;
  }

  DBRole itemrole;
  if (!deserializeDBRole(*role, &itemrole)) {
    return false;
  }
  setRole(itemrole);
  auto* status = data.get_ptr("Status");
  if (status == nullptr) {
    return false;
  }

  ShardServiceStatus itemstatus;
  if (!deserializeShardServiceStatus(*status, &itemstatus)) {
    return false;
  }
  setStatus(itemstatus);
  auto* partitions = data.get_ptr("Partitions");
  if (partitions == nullptr || !partitions->isArray()) {
    return false;
  }

  std::vector<PartitionMetaInfo> vec_partitions;
  for (size_t i = 0; i < partitions->size(); i++) {
    if (!partitions->at(i).isObject()) {
      return false;
    }

    PartitionMetaInfo obj_partitions;
    if (!obj_partitions.deserialize(partitions->at(i))) {
      return false;
    }
    vec_partitions.push_back(obj_partitions);
  }
  setPartitions(vec_partitions);

  return true;
}


}  // namespace laser
