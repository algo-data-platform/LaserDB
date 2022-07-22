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
 * @author liubang <it.liubang@gmail.com>
 */

#pragma once

#include "common/service_router/router.h"
#include "folly/Synchronized.h"
#include "laser_entity.h"

namespace laser {

using TableSchemasMap = std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>;
using TableTrafficRestrictionMap = std::unordered_map<uint64_t, std::shared_ptr<TableTrafficRestrictionConfig>>;
using NotifyDatabaseAndClusterUpdate =
    folly::Function<void(const NodeShardList&, const std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>&)>;
using NotifyRocksDbConfigUpdate =
    folly::Function<void(const std::shared_ptr<NodeConfig>, const TableConfigList&, const TableSchemasMap&)>;
using NotifyTrafficRestrictionUpdate = folly::Function<void(const TableTrafficRestrictionMap&)>;

class ConfigManager {
 public:
  explicit ConfigManager(const std::shared_ptr<service_router::Router> router)
      : router_(router),
        node_config_(nullptr),
        table_config_list_(std::make_shared<TableConfigList>()),
        use_manually_set_config_(false),
        is_edge_node_(false) {}
  virtual ~ConfigManager() = default;
  virtual void init(const std::string& service_name, const std::string& group_name = "", uint32_t node_id = 0);
  virtual folly::Optional<std::shared_ptr<TableSchema>> getTableSchema(const std::string& database_name,
                                                                       const std::string& table_name);
  virtual std::shared_ptr<TableSchemasMap> getTableSchemas() const;
  virtual std::shared_ptr<TableTrafficRestrictionMap> getTrafficRestrictionConfig();

  virtual folly::Optional<NodeShardList> getNodeShardList(const std::string& group_name, uint32_t node_id);

  // deprecated, use getTableSchemas instead
  virtual inline folly::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>> getAllTableSchemas() {
    return table_schemas_;
  }

  virtual inline folly::Optional<uint32_t> getShardNumber() {
    if (cluster_info_) {
      return cluster_info_->getShardNumber();
    }

    return folly::none;
  }

  virtual folly::Optional<uint32_t> getShardNumber(const std::string& dc);

  virtual void updateConfig(const std::unordered_map<std::string, std::string>& configs, const std::string& group_name,
                            uint32_t node_id, bool config_from_service_router = true);
  virtual void initOverwriteConfig(const std::unordered_map<std::string, std::string>& configs,
                                   const std::string& group_name, const uint32_t node_id,
                                   bool config_from_service_router = true);
  virtual void updateDatabase(const std::string& value);
  virtual void updateClusterInfo(const std::string& value, const std::string& group_name, const uint32_t node_id);
  virtual void updateNodeConfig(const std::string& node_configs_value, const std::string& node_config_list_value,
                                const std::string& group_name, const uint32_t node_id);
  virtual void updateTableConfigList(const std::string& table_config_list_value);
  virtual void updateTrafficRestrictionConfig(const std::string& traffic_restriction_value);

  virtual inline void subscribe(const std::string& group_name, uint32_t node_id,
                                NotifyDatabaseAndClusterUpdate notify) {
    update_subscribers_.withWLock([&group_name, node_id, &notify, this](auto& subscribes) {
      subscribes[getNodeListHash(group_name, node_id)] =
          std::make_shared<NotifyDatabaseAndClusterDetail>(group_name, node_id, std::move(notify));
    });
  }

  virtual inline void subscribeRocksDbConfig(NotifyRocksDbConfigUpdate notify) {
    notify_rocksdb_config_update_ = std::move(notify);
  }

  virtual inline void subscribeTrafficRestrictionConfig(NotifyTrafficRestrictionUpdate notify) {
    notify_traffic_restriction_update_ = std::move(notify);
  }

  virtual inline void unsubscribe(const std::string& group_name, uint32_t node_id) {
    update_subscribers_.withWLock([&group_name, node_id, this](auto& subscribes) {
      uint64_t key = getNodeListHash(group_name, node_id);
      if (subscribes.find(key) != subscribes.end()) {
        subscribes.erase(key);
      }
    });
  }

  virtual inline std::shared_ptr<service_router::Router> getRouter() { return router_.lock(); }
  virtual uint64_t getTableSchemaHash(const std::string& database_name, const std::string& table_name);
  virtual void setUseManuallySetConfig(bool use_manually_set_config) {
    use_manually_set_config_ = use_manually_set_config;
  }
  virtual bool isEdgeNode() { return is_edge_node_; }

 private:
  std::weak_ptr<service_router::Router> router_;
  folly::Synchronized<TableSchemasMap> table_schemas_;
  std::shared_ptr<ClusterInfo> cluster_info_;
  folly::Synchronized<std::unordered_map<uint64_t, NodeShardList>> node_shard_lists_;
  folly::Synchronized<std::shared_ptr<NodeConfig>> node_config_;
  folly::Synchronized<std::shared_ptr<TableConfigList>> table_config_list_;
  folly::Synchronized<TableTrafficRestrictionMap> traffic_restriction_config_;
  folly::Synchronized<std::unordered_map<std::string, DataCenter>> dcs_;
  std::atomic_bool use_manually_set_config_;
  std::atomic_bool is_edge_node_;

  class NotifyDatabaseAndClusterDetail {
   public:
    NotifyDatabaseAndClusterDetail(const std::string& group_name, uint32_t node_id,
                                   NotifyDatabaseAndClusterUpdate notify_callback)
        : group_name_(group_name), node_id_(node_id), notify_callback_(std::move(notify_callback)) {}
    ~NotifyDatabaseAndClusterDetail() = default;

    void notify(const NodeShardList& list, const std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>& tables) {
      notify_callback_(list, tables);
    }

   private:
    std::string group_name_;
    uint32_t node_id_;
    NotifyDatabaseAndClusterUpdate notify_callback_;
  };
  folly::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<NotifyDatabaseAndClusterDetail>>>
      update_subscribers_;
  NotifyRocksDbConfigUpdate notify_rocksdb_config_update_;
  NotifyTrafficRestrictionUpdate notify_traffic_restriction_update_;

  uint64_t getNodeListHash(const std::string& group_name, uint32_t node_id);
  void upateDatabaseAndCluster();
};

}  // namespace laser
