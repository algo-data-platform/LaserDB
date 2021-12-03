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
 * @author liubang <it.liubang@gmail.com>
 */

#include "config_manager.h"

namespace laser {

constexpr static char LASER_CONFIG_DATABASE_TABLE_SCHEMA_DATA[] = "database_table_schema_data";
constexpr static char LASER_CONFIG_CLUSTER_INFO[] = "cluster_info_data";
constexpr static char LASER_CONFIG_NODE_CONFIG_LIST[] = "node_config_list_data";
constexpr static char LASER_CONFIG_TABLE_CONFIG_LIST[] = "table_config_list_data";
constexpr static char LASER_CONFIG_ROCKSDB_NODE_CONFIGS[] = "rocksdb_node_configs_data";
constexpr static char LASER_CONFIG_TRAFFIC_RESTRICTION_CONFIGS[] = "traffic_restriction_data";

void ConfigManager::init(const std::string& service_name, const std::string& group_name, uint32_t node_id) {
  auto router = router_.lock();
  if (!router) {
    return;
  }
  router->waitForConfig(service_name);
  auto config_vals = router->getConfigs(service_name);
  updateConfig(config_vals, group_name, node_id);

  router->subscribeConfig(service_name,
                          [this, group_name, node_id](const std::string&, const service_router::ServiceConfig& config) {
                            updateConfig(config.getConfigs(), group_name, node_id);
                          });
}

void ConfigManager::updateConfig(const std::unordered_map<std::string, std::string>& configs,
                                 const std::string& group_name, uint32_t node_id, bool config_from_service_router) {
  initOverwriteConfig(configs, group_name, node_id, config_from_service_router);

  if (notify_rocksdb_config_update_) {
    folly::synchronized(
        [&](auto node_config, auto table_config_list, auto table_schemas) {
          notify_rocksdb_config_update_(*node_config, **table_config_list, *table_schemas);
        },
        folly::rlock(node_config_), folly::rlock(table_config_list_), folly::rlock(table_schemas_));
  }

  if (notify_traffic_restriction_update_) {
    traffic_restriction_config_.withRLock(
        [this](auto& traffic_restriction_config) { notify_traffic_restriction_update_(traffic_restriction_config); });
  }
  upateDatabaseAndCluster();
}

void ConfigManager::initOverwriteConfig(const std::unordered_map<std::string, std::string>& configs,
                                        const std::string& group_name, const uint32_t node_id,
                                        bool config_from_service_router) {
  for (auto& config : configs) {
    if (config.first == LASER_CONFIG_DATABASE_TABLE_SCHEMA_DATA) {
      updateDatabase(config.second);
      continue;
    }
    if (config.first == LASER_CONFIG_CLUSTER_INFO) {
      if (use_manually_set_config_ && config_from_service_router) {
        continue;
      }
      updateClusterInfo(config.second, group_name, node_id);
      continue;
    }

    if (config.first == LASER_CONFIG_ROCKSDB_NODE_CONFIGS) {
      auto found_node_config_list = configs.find(LASER_CONFIG_NODE_CONFIG_LIST);
      if (found_node_config_list != configs.end()) {
        updateNodeConfig(config.second, found_node_config_list->second, group_name, node_id);
      }
      continue;
    }

    if (config.first == LASER_CONFIG_TABLE_CONFIG_LIST) {
      updateTableConfigList(config.second);
      continue;
    }

    if (config.first == LASER_CONFIG_TRAFFIC_RESTRICTION_CONFIGS) {
      updateTrafficRestrictionConfig(config.second);
      continue;
    }
  }
}

folly::Optional<std::shared_ptr<TableSchema>> ConfigManager::getTableSchema(const std::string& database_name,
                                                                            const std::string& table_name) {
  std::shared_ptr<TableSchema> table_schema;
  bool has_table = table_schemas_.withRLock([this, &table_name, &database_name, &table_schema](auto& table_schemas) {
    uint64_t key = getTableSchemaHash(database_name, table_name);
    if (table_schemas.find(key) != table_schemas.end()) {
      table_schema = table_schemas.at(key);
      return true;
    }
    return false;
  });

  if (has_table) {
    return table_schema;
  }
  return folly::none;
}

std::shared_ptr<TableSchemasMap> ConfigManager::getTableSchemas() const {
  std::shared_ptr<TableSchemasMap> new_table_schemas = nullptr;
  table_schemas_.withRLock([this, &new_table_schemas](auto& table_schemas) {
    if (table_schemas.empty()) {
      return;
    }
    new_table_schemas = std::make_shared<TableSchemasMap>();
    for (auto& schema : table_schemas) {
      auto new_schema = std::make_shared<TableSchema>(*schema.second);
      new_table_schemas->insert(std::make_pair(schema.first, new_schema));
    }
  });
  return new_table_schemas;
}

std::shared_ptr<TableTrafficRestrictionMap> ConfigManager::getTrafficRestrictionConfig() {
  std::shared_ptr<TableTrafficRestrictionMap> temp_restriction = nullptr;
  traffic_restriction_config_.withRLock([&temp_restriction](auto& traffic_restriction) {
    if (traffic_restriction.empty()) {
      return;
    }
    temp_restriction = std::make_shared<TableTrafficRestrictionMap>();
    for (auto& iter : traffic_restriction) {
      auto table_traffic_restriction = std::make_shared<TableTrafficRestrictionConfig>(*iter.second);
      temp_restriction->insert(std::make_pair(iter.first, table_traffic_restriction));
    }
  });
  return temp_restriction;
}

folly::Optional<NodeShardList> ConfigManager::getNodeShardList(const std::string& group_name, uint32_t node_id) {
  NodeShardList shard_list;
  bool has_node = node_shard_lists_.withRLock([this, &group_name, node_id, &shard_list](auto& nodes) {
    uint64_t key = getNodeListHash(group_name, node_id);
    if (nodes.find(key) != nodes.end()) {
      shard_list = nodes.at(key);
      return true;
    }
    return false;
  });

  if (has_node) {
    return shard_list;
  }
  return folly::none;
}

void ConfigManager::updateDatabase(const std::string& value) {
  folly::dynamic dy_database_config;
  if (!common::fromJson(&dy_database_config, value)) {
    LOG(ERROR) << "Json parse is fail, database config, value:" << value;
    return;
  }

  if (!dy_database_config.isArray()) {
    LOG(ERROR) << "Json parse is fail, database config is not list,  value:" << value;
    return;
  }

  std::vector<DatabaseSchema> database_schemas;
  for (size_t i = 0; i < dy_database_config.size(); i++) {
    if (!dy_database_config.at(i).isObject()) {
      LOG(ERROR) << "Database config item is not object, level tag: value:" << value << ", item id:" << i;
      return;
    }

    DatabaseSchema database_schema;
    if (!database_schema.deserialize(dy_database_config.at(i))) {
      LOG(ERROR) << "Database config item deserialize fail, level tag: value:" << value << ", item id:" << i;
      return;
    }
    database_schemas.push_back(database_schema);
  }

  table_schemas_.withWLock([&database_schemas, this](auto& table_schemas) {
    // node 或者 group 级别会覆盖所有的表信息
    table_schemas.clear();
    for (auto& database : database_schemas) {
      auto tables = database.getTables();
      for (auto& table : tables) {
        uint64_t key = getTableSchemaHash(database.getDatabaseName(), table.getTableName());
        VLOG(5) << "Add database:" << database.getDatabaseName() << " table:" << table.getTableName();
        table.setDatabaseName(database.getDatabaseName());
        table_schemas[key] = std::make_shared<TableSchema>(table);
      }
    }
  });
}

void ConfigManager::updateClusterInfo(const std::string& value, const std::string& group_name, const uint32_t node_id) {
  folly::dynamic dy_cluster_info;
  if (!common::fromJson(&dy_cluster_info, value)) {
    LOG(ERROR) << "Json parse is fail, cluster info config:" << value;
    return;
  }
  ClusterInfo cluster_info;
  if (!cluster_info.deserialize(dy_cluster_info)) {
    LOG(ERROR) << "Cluster info deserialize fail, value:" << value;
    return;
  }

  for (auto& group : cluster_info.getGroups()) {
    for (auto& node : group.getNodes()) {
      if (group.getGroupName() == group_name && node.getNodeId() == node_id) {
        is_edge_node_ = node.getIsEdgeNode();
      }
      uint64_t key = getNodeListHash(group.getGroupName(), node.getNodeId());
      node_shard_lists_.withWLock([key, &node](auto& node_shard_lists) { node_shard_lists[key] = node; });
    }
  }
  for (auto& dc : cluster_info.getDcs()) {
    dcs_.withWLock([&dc](auto& dcs) { dcs[dc.getName()] = dc; });
  }
  cluster_info_ = std::make_shared<ClusterInfo>(cluster_info);
}

void ConfigManager::updateNodeConfig(const std::string& node_configs_value, const std::string& node_config_list_value,
                                     const std::string& group_name, const uint32_t node_id) {
  folly::dynamic dy_node_configs;
  if (!common::fromJson(&dy_node_configs, node_configs_value)) {
    LOG(ERROR) << "Json parse node configs failed, value : " << node_configs_value;
    return;
  }

  RocksdbNodeConfigs node_configs;
  if (!node_configs.deserialize(dy_node_configs)) {
    LOG(ERROR) << "Rocksdb node configs deserialize failed, value: " << node_configs_value;
    return;
  }

  folly::dynamic dy_node_config_list;
  if (!common::fromJson(&dy_node_config_list, node_config_list_value)) {
    LOG(ERROR) << "Json parse node config list failed, value: " << node_config_list_value;
    return;
  }

  NodeConfigList node_config_list;
  if (!node_config_list.deserialize(dy_node_config_list)) {
    LOG(ERROR) << "Node config list deserialize failed, value: " << node_config_list_value;
    return;
  }

  auto node_config_key = folly::to<std::string>(group_name, "#", node_id);
  auto raw_node_configs = node_configs.getRocksdbNodeConfigs();
  auto node_configs_iter = raw_node_configs.find(node_config_key);
  if (node_configs_iter == raw_node_configs.end()) {
    LOG(ERROR) << "Can not find node config in rocksdb node configs, value: " << node_configs_value
               << ", node config key: " << node_config_key;
    return;
  }
  auto node_config_name = node_configs_iter->second;

  auto raw_config_list = node_config_list.getNodeConfigList();
  auto config_list_iter = raw_config_list.find(node_config_name);
  if (config_list_iter == raw_config_list.end()) {
    LOG(ERROR) << "Can not find config item in node config list: value: " << node_config_list_value
               << ", node config name: " << node_config_name;
    return;
  }

  node_config_.withWLock(
      [&config_list_iter](auto& node_config) { node_config = std::make_shared<NodeConfig>(config_list_iter->second); });
}

void ConfigManager::updateTableConfigList(const std::string& table_config_list_value) {
  folly::dynamic dy_table_config_list;
  if (!common::fromJson(&dy_table_config_list, table_config_list_value)) {
    LOG(ERROR) << "Json parse table config list failed, value: " << table_config_list_value;
    return;
  }

  auto table_config_list = std::make_shared<TableConfigList>();
  if (!table_config_list->deserialize(dy_table_config_list)) {
    LOG(ERROR) << "Table config list deserialize failed, value: " << table_config_list_value;
    return;
  }

  table_config_list_.withWLock([&table_config_list](auto& list) { list = table_config_list; });
}

void ConfigManager::updateTrafficRestrictionConfig(const std::string& traffic_restriction_value) {
  folly::dynamic dy_traffic_restriction;
  if (!common::fromJson(&dy_traffic_restriction, traffic_restriction_value)) {
    LOG(ERROR) << "Json parse traffice restriction config, value: " << traffic_restriction_value;
    return;
  }

  if (!dy_traffic_restriction.isArray()) {
    LOG(ERROR) << "Json parse failed, traffic restriction config is not list, value: " << traffic_restriction_value;
    return;
  }

  std::vector<DatabaseTrafficRestrictionConfig> db_traffic_restrictions;
  for (size_t i = 0; i < dy_traffic_restriction.size(); i++) {
    if (!dy_traffic_restriction.at(i).isObject()) {
      LOG(ERROR) << "Traffic restriction item is not object, vaule: " << traffic_restriction_value
                 << ", item id: " << i;
      return;
    }
    DatabaseTrafficRestrictionConfig db_restriction_config;
    if (!db_restriction_config.deserialize(dy_traffic_restriction.at(i))) {
      LOG(ERROR) << "Traffic restriction item deserialize failed, value: " << traffic_restriction_value
                 << ", item id: " << i;
      return;
    }
    db_traffic_restrictions.emplace_back(db_restriction_config);
  }

  traffic_restriction_config_.withWLock([&db_traffic_restrictions, this](auto& traffic_restrictions) {
    traffic_restrictions.clear();
    for (auto& db_restriction : db_traffic_restrictions) {
      auto table_restrictions = db_restriction.getTables();
      for (auto& table_restriction : table_restrictions) {
        uint64_t key = getTableSchemaHash(db_restriction.getDatabaseName(), table_restriction.getTableName());
        traffic_restrictions[key] = std::make_shared<TableTrafficRestrictionConfig>(table_restriction);
      }
    }
  });
}

uint64_t ConfigManager::getTableSchemaHash(const std::string& database_name, const std::string& table_name) {
  uint64_t key = CityHash64WithSeed(database_name.data(), database_name.size(), 0);
  return CityHash64WithSeed(table_name.data(), table_name.size(), key);
}

uint64_t ConfigManager::getNodeListHash(const std::string& group_name, uint32_t node_id) {
  return CityHash64WithSeed(group_name.data(), group_name.size(), node_id);
}

void ConfigManager::upateDatabaseAndCluster() {
  auto ret = folly::acquireLocked(table_schemas_, update_subscribers_);
  auto& table_schemas = std::get<0>(ret);
  auto& subscribes = std::get<1>(ret);

  if (!cluster_info_) {
    LOG(ERROR) << "Cluster info not exists, need wait sync cluster info.";
    return;
  }

  for (auto& group : cluster_info_->getGroups()) {
    for (auto& node : group.getNodes()) {
      uint64_t key = getNodeListHash(group.getGroupName(), node.getNodeId());
      if (subscribes->find(key) == subscribes->end()) {
        VLOG(5) << "Database and cluster info update subscribes is not exists, group_name:" << group.getGroupName()
                << " node_id: " << node.getNodeId();
        continue;
      }

      auto node_list = getNodeShardList(group.getGroupName(), node.getNodeId());
      if (!node_list) {
        VLOG(5) << "Database and cluster info update, but shard list get fail, group_name:" << group.getGroupName()
                << " node_id: " << node.getNodeId();
        continue;
      }

      subscribes->at(key)->notify(node_list.value(), *table_schemas);
    }
  }
}

folly::Optional<uint32_t> ConfigManager::getShardNumber(const std::string& dc) {
  uint32_t shard_number;
  bool ret = dcs_.withRLock([&dc, &shard_number](auto& dcs) {
    auto it = dcs.find(dc);
    if (it != dcs.end()) {
      shard_number = it->second.getShardNumber();
      return true;
    }
    return false;
  });
}

}  // namespace laser
