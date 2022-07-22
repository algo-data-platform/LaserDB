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

#include "database_manager.h"
#include "engine/rocksdb.h"

#include "common/service_router/router.h"

namespace laser {

DEFINE_int32(hdfs_monitor_sync_thread_nums, 16, "Laser sync hdfs file thread nums");
DEFINE_int32(wdt_replicator_manager_thread_nums, 16, "Laser wdt replicator manager thread nums");
DEFINE_int32(delay_set_available_seconds, 20, "Delay seconds to set service available after loading database");

constexpr static char WDT_REPLICATOR_NAME[] = "laser_base_data_replicator";
constexpr static char LASER_METRICS_MODULE_NAME_FOR_ROCKSDB_TABLE[] = "rocksdb_table";
constexpr static char LASER_METRICS_MODULE_NAME_FOR_SHARD[] = "shard";
constexpr static char LASER_METRICS_METRIC_NAME_FOR_SHARD[] = "stat";
constexpr static char LASER_METRIC_NAME_FOR_PARTITION_NUM[] = "partition_number";
constexpr static char LASER_METRICS_TAG_TABLE_NAME[] = "table_name";
constexpr static char LASER_METRICS_TAG_DB_NAME[] = "database_name";
constexpr static char LASER_METRICS_TAG_ROLE_NAME[] = "role";
constexpr static char LASER_METRICS_TAG_SHARD_ID[] = "shard_id";
constexpr static char LASER_METRICS_TAG_NODE_ID[] = "node_id";
constexpr static char LASER_METRICS_TAG_GROUP_ID[] = "group_id";
constexpr static char LASER_METRICS_TAG_PROPERTY[] = "property";
constexpr static uint32_t METRIC_ROCKSDB_STAT_INTERVAL = 10000;

void DatabaseManager::init(uint32_t loader_thread_nums, const std::string& replicator_service_name,
                           const std::string& replicator_host, uint32_t replicator_port) {
  replicator_service_name_ = replicator_service_name;
  replicator_host_ = replicator_host;
  loader_thread_pool_ = createLoaderThreadPool(loader_thread_nums);
  partition_manager_ = createPartitionManager();
  partition_manager_->subscribe([this](auto& mount_partitions, auto& unmount_partitions) {
    loader_thread_pool_->add(
        [this, mount_partitions, unmount_partitions]() { updatePartitions(mount_partitions, unmount_partitions); });
  });
  hdfs_monitor_manager_ = createHdfsMonitorManager();

  timer_thread_ = std::make_unique<DatabaseManagerTimerThread>();
  self_hold_metrics_thread_ = std::make_unique<metrics::MetricsThread>("self_hold_metrics_thread");

  // init rocks db config factory
  rocksdb_config_factory_ = createRocksDbConfigFactory();
  rocksdb_config_factory_->init([this]() { rocksdb_config_semaphore_.post(); }, timer_thread_->getEventBase());

  // init replicator manager
  replicator_manager_ = createReplicatorManager();
  replicator_manager_->init(replicator_service_name, replicator_host, replicator_port, getNodeHash(), node_dc_);

  // init wdt replicator
  wdt_manager_ = createWdtReplicatorManager(FLAGS_wdt_replicator_manager_thread_nums);

  setShardMetrics();
}

DatabaseManager::~DatabaseManager() {
  if (loader_thread_pool_) {
    loader_thread_pool_->stop();
  }
}

std::shared_ptr<folly::CPUThreadPoolExecutor> DatabaseManager::createLoaderThreadPool(uint32_t thread_nums) {
  return std::make_shared<folly::CPUThreadPoolExecutor>(thread_nums,
                                                        std::make_shared<folly::NamedThreadFactory>("LoaderDataPool"));
}

std::shared_ptr<PartitionManager> DatabaseManager::createPartitionManager() {
  return std::make_shared<PartitionManager>(config_manager_, group_name_, node_id_, node_dc_);
}

std::shared_ptr<hdfs::HdfsMonitorManager> DatabaseManager::createHdfsMonitorManager() {
  return std::make_shared<hdfs::HdfsMonitorManager>(FLAGS_hdfs_monitor_sync_thread_nums);
}

std::shared_ptr<RocksDbConfigFactory> DatabaseManager::createRocksDbConfigFactory() {
  return std::make_shared<RocksDbConfigFactory>(config_manager_);
}

std::shared_ptr<DatabaseMetaInfo> DatabaseManager::createDatabaseMetaInfo() {
  return std::make_shared<DatabaseMetaInfo>();
}

std::shared_ptr<ReplicatorManager> DatabaseManager::createReplicatorManager() {
  return std::make_shared<ReplicatorManager>();
}

std::shared_ptr<WdtReplicatorManager> DatabaseManager::createWdtReplicatorManager(uint32_t thread_nums) {
  return std::make_shared<laser::WdtReplicatorManager>(WDT_REPLICATOR_NAME, thread_nums);
}

void DatabaseManager::updatePartitions(const PartitionPtrSet& mount_partitions,
                                       const PartitionPtrSet& unmount_partitions) {
  if (!meta_info_inited_.test_and_set()) {
    rocksdb_config_semaphore_.wait();
    // init database meta info
    auto op_rocksdb_options = rocksdb_config_factory_->getDefaultOptions();
    if (!op_rocksdb_options.hasValue()) {
      LOG(ERROR) << "Can not get default rocksdb options!";
      DCHECK(false);
      return;
    }
    database_meta_info_ = createDatabaseMetaInfo();
    database_meta_info_->init(op_rocksdb_options.value());
    delaySetAvailable();
  }

  updateServerShard();
  semaphore_.post();

  for (auto& partition : mount_partitions) {
    VLOG(3) << "Mount partition:" << *partition;
    auto partition_handler = getOrCreatePartitionHandler(partition);
    auto monitor = getOrCreateTableMonitor(partition);
    if (partition_handler->getPartition()->getRole() != partition->getRole()) {  // 角色变化
      if (partition->getRole() == DBRole::LEADER) {
        monitor->addPartition(partition->getPartitionId());
      } else {
        monitor->removePartition(partition->getPartitionId());
      }
      // update partition
      partition_handler->setPartition(partition);
    } else {
      if (partition->getRole() == DBRole::LEADER) {
        monitor->addPartition(partition->getPartitionId());
      }
    }
    if (partition_handler->getPartition()->getDc() != partition->getDc()) {
      partition_handler->setPartition(partition);
    }
  }

  for (auto& partition : unmount_partitions) {
    VLOG(3) << "Unmount partition:" << *partition;
    auto monitor = getOrCreateTableMonitor(partition);
    monitor->removePartition(partition->getPartitionId());
    removePartitionHandler(partition);
  }
}

void DatabaseManager::updateServerShard() {
  if (has_api_server_) {
    std::vector<uint32_t> leaderShards;
    std::vector<uint32_t> followerShards;
    std::shared_ptr<std::vector<int64_t>> partition_list = std::make_shared<std::vector<int64_t>>();
    if (config_manager_->isEdgeNode()) {
      partition_list = partition_manager_->getPartitionHashList();
    } else {
      std::vector<uint32_t> unavailable_shards;
      unavailable_shards_.withRLock([&unavailable_shards](auto& shards) { unavailable_shards = shards; });

      for (uint32_t shard_id : partition_manager_->getLeaderShardList()) {
        auto iter = std::find(unavailable_shards.begin(), unavailable_shards.end(), shard_id);
        if (iter == unavailable_shards.end()) {
          leaderShards.push_back(shard_id);
        }
      }
      for (uint32_t shard_id : partition_manager_->getFollowerShardList()) {
        auto iter = std::find(unavailable_shards.begin(), unavailable_shards.end(), shard_id);
        if (iter == unavailable_shards.end()) {
          followerShards.push_back(shard_id);
        }
      }
    }

    config_manager_->getRouter()->setAvailableShardList(api_server_, leaderShards);
    config_manager_->getRouter()->setFollowerAvailableShardList(api_server_, followerShards);
    config_manager_->getRouter()->setPartitionList(api_server_, *partition_list);
    config_manager_->getRouter()->setIsEdgeNode(api_server_, config_manager_->isEdgeNode());
  }

  if (replicator_manager_) {
    replicator_manager_->setShardList(partition_manager_->getLeaderShardList(),
                                      partition_manager_->getFollowerShardList());
  }
}

void DatabaseManager::setUnavailableShards(const std::vector<uint32_t> shard_ids) {
  unavailable_shards_.withWLock([&shard_ids](auto& shards) { shards = shard_ids; });

  updateServerShard();
}

void DatabaseManager::triggerBase(const std::string& database_name, const std::string& table_name,
                                  const std::string& version) {
  auto monitor = getOrCreateTableMonitor(database_name, table_name);
  monitor->updateBase(version);
}

void DatabaseManager::triggerDelta(const std::string& database_name, const std::string& table_name,
                                   const std::string& base_version, const std::vector<std::string>& delta_versions) {
  auto monitor = getOrCreateTableMonitor(database_name, table_name);
  monitor->updateDelta(base_version, delta_versions);
}

void DatabaseManager::triggerBaseDataReplication(const std::string& database_name, const std::string& table_name) {
  auto table_schemas = config_manager_->getTableSchemas();
  if (!table_schemas) {
    return;
  }
  auto table_hash = config_manager_->getTableSchemaHash(database_name, table_name);
  auto table_schema = table_schemas->find(table_hash);
  if (table_schema != table_schemas->end()) {
    uint32_t partition_number = table_schema->second->getPartitionNumber();
    for (int id = 0; id < partition_number; ++id) {
      auto partition = std::make_shared<Partition>(database_name, table_name, id);
      auto partition_handler = getPartitionHandler(partition);
      if (partition_handler.hasValue()) {
        partition_handler.value()->forceBaseDataReplication();
      }
    }
  }
}

std::shared_ptr<TableMonitor> DatabaseManager::getOrCreateTableMonitor(const std::shared_ptr<Partition>& partition) {
  return getOrCreateTableMonitor(partition->getDatabaseName(), partition->getTableName());
}

std::shared_ptr<TableMonitor> DatabaseManager::getOrCreateTableMonitor(const std::string& database_name,
                                                                       const std::string& table_name) {
  return table_monitors_.withULockPtr([&database_name, &table_name, this](auto ulock) {
    uint64_t table_hash = getTableHash(database_name, table_name);
    if (ulock->find(table_hash) != ulock->end()) {
      return ulock->at(table_hash);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto monitor = std::make_shared<TableMonitor>(database_name, table_name, hdfs_monitor_manager_);
    monitor->init();
    monitor->subscribeBaseLoad([this](const std::shared_ptr<Partition>& partition, const std::string& version) {
      loader_thread_pool_->add([partition, version, this]() {
        auto partition_handler = getPartitionHandler(partition);
        if (partition_handler.hasValue()) {
          partition_handler.value()->loadBaseData(version);
        }
      });
    });
    monitor->subscribeDeltaLoad([this](const std::shared_ptr<Partition>& partition, const std::string& version,
                                       const std::vector<std::string>& delta_versions) {
      loader_thread_pool_->add([partition, version, delta_versions, this]() {
        auto partition_handler = getPartitionHandler(partition);
        if (partition_handler.hasValue()) {
          partition_handler.value()->loadDeltaData(version, delta_versions);
        }
      });
    });

    // 设置表监控项
    setTableMetrics(database_name, table_name);
    (*wlock)[table_hash] = monitor;
    return wlock->at(table_hash);
  });
}

std::shared_ptr<PartitionHandler> DatabaseManager::getOrCreatePartitionHandler(
    const std::shared_ptr<Partition>& partition) {
  return partition_handlers_.withULockPtr([&partition, this](auto ulock) {
    int64_t hash_key = partition->getPartitionHash();
    if (ulock->find(hash_key) != ulock->end()) {
      return ulock->at(hash_key);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto table = config_manager_->getTableSchema(partition->getDatabaseName(), partition->getTableName());
    uint64_t ttl = 0;
    if (table) {
      ttl = table.value()->getTtl();
    }
    auto engine_options = std::make_shared<RocksDbEngineOptions>();
    engine_options->ttl = ttl;
    auto handler = std::make_shared<PartitionHandler>(
        partition, this, database_meta_info_.get(), replicator_manager_.get(), wdt_manager_,
        timer_thread_->getEventBase(), engine_options, self_hold_metrics_thread_->getEventBase());
    handler->init();
    (*wlock)[hash_key] = handler;
    return wlock->at(hash_key);
  });
}

folly::Optional<std::shared_ptr<PartitionHandler>> DatabaseManager::getPartitionHandler(
    const std::shared_ptr<Partition>& partition) {
  return partition_handlers_.withRLock(
      [&partition, this](auto& handlers) -> folly::Optional<std::shared_ptr<PartitionHandler>> {
        int64_t hash_key = partition->getPartitionHash();
        if (handlers.find(hash_key) != handlers.end()) {
          return handlers.at(hash_key);
        }
        return folly::none;
      });
}

void DatabaseManager::removePartitionHandler(const std::shared_ptr<Partition>& partition) {
  partition_handlers_.withWLock([&partition, this](auto& handlers) {
    int64_t hash_key = partition->getPartitionHash();

    if (handlers.find(hash_key) != handlers.end()) {
      handlers.erase(hash_key);
    }
  });
}

folly::Optional<std::weak_ptr<RocksDbEngine>> DatabaseManager::getDatabaseHandler(
    const std::shared_ptr<Partition>& partition) {
  auto partition_handler = getPartitionHandler(partition);
  if (!partition_handler.hasValue()) {
    return folly::none;
  }
  auto db = partition_handler.value()->getDatabase();
  if (db.lock()) {
    return db;
  }

  return folly::none;
}

void DatabaseManager::getTableMetaInfo(TableMetaInfo* table_info, const std::string& database_name,
                                       const std::string& table_name) {
  table_info->setGroupName(group_name_);
  table_info->setNodeId(node_id_);
  std::vector<PartitionMetaInfo> partitions;
  auto table = config_manager_->getTableSchema(database_name, table_name);
  if (!table) {
    VLOG(5) << "Not exists database:" << database_name << " table:" << table_name;
    return;
  }

  uint32_t partition_number = table.value()->getPartitionNumber();
  for (int i = 0; i < partition_number; i++) {
    std::shared_ptr<Partition> partition = std::make_shared<Partition>(database_name, table_name, i);
    auto partition_handler = getPartitionHandler(partition);
    if (!partition_handler.hasValue()) {
      VLOG(5) << "Not exists database:" << database_name << " table:" << table_name << " partition id:" << i;
      continue;
    }

    PartitionMetaInfo partition_meta_info;
    partition_handler.value()->getPartitionMetaInfo(&partition_meta_info);
    partitions.push_back(partition_meta_info);
  }
  table_info->setPartitions(partitions);
}

int64_t DatabaseManager::getShardUniqueId(uint32_t shard_id, const DBRole& db_role) {
  std::string role = toStringDBRole(db_role);
  return CityHash64WithSeed(role.data(), role.size(), shard_id);
}

void DatabaseManager::getShardMetaInfo(std::vector<std::shared_ptr<ShardMetaInfo>>* shards) {
  auto serverStatus = config_manager_->getRouter()->getStatus(api_server_);
  std::vector<uint32_t> unavailable_shards;
  unavailable_shards_.withRLock([&unavailable_shards](auto& shards) { unavailable_shards = shards; });
  std::unordered_map<int64_t, std::shared_ptr<ShardMetaInfo>> shard_metadata;
  for (uint32_t shard_id : partition_manager_->getLeaderShardList()) {
    int64_t key = getShardUniqueId(shard_id, DBRole::LEADER);
    auto meta = std::make_shared<ShardMetaInfo>();
    meta->setShardId(shard_id);
    meta->setRole(DBRole::LEADER);
    shard_metadata[key] = meta;
  }
  for (uint32_t shard_id : partition_manager_->getFollowerShardList()) {
    int64_t key = getShardUniqueId(shard_id, DBRole::FOLLOWER);
    auto meta = std::make_shared<ShardMetaInfo>();
    meta->setShardId(shard_id);
    meta->setRole(DBRole::FOLLOWER);
    shard_metadata[key] = meta;
  }
  for (auto& shard_info : shard_metadata) {
    auto iter = std::find(unavailable_shards.begin(), unavailable_shards.end(), shard_info.second->getShardId());
    if (iter == unavailable_shards.end()) {
      shard_info.second->setStatus(ShardServiceStatus::AVAILABLE);
    } else {
      shard_info.second->setStatus(ShardServiceStatus::UNAVAILABLE);
    }
    if (serverStatus && serverStatus.value() == service_router::ServerStatus::UNAVAILABLE) {
      shard_info.second->setStatus(ShardServiceStatus::UNAVAILABLE);
    }
  }

  std::unordered_map<int64_t, std::vector<PartitionMetaInfo>> partition_metadata;
  std::unordered_map<int64_t, std::shared_ptr<PartitionHandler>> partitions;
  partition_handlers_.withRLock([&partitions](auto& handlers) { partitions = handlers; });
  for (auto& partition : partitions) {
    PartitionMetaInfo partition_meta_info;
    partition.second->getPartitionMetaInfo(&partition_meta_info);
    auto partition_entity = partition.second->getPartition();
    int64_t key = getShardUniqueId(partition_entity->getShardId(), partition_entity->getRole());
    auto shardInfo = partition_metadata.find(key);
    if (shardInfo == partition_metadata.end()) {
      partition_metadata[key] = {};
    }
    partition_metadata[key].push_back(partition_meta_info);
  }

  for (auto& partition : partition_metadata) {
    if (shard_metadata.find(partition.first) == shard_metadata.end()) {
      continue;
    }
    shard_metadata[partition.first]->setPartitions(partition.second);
  }
  for (auto& shard : shard_metadata) {
    shards->push_back(shard.second);
  }
}

void DatabaseManager::monitorSwitch(bool switch_flag) { hdfs_monitor_manager_->monitorSwitch(switch_flag); }

void DatabaseManager::cleanUnusedPartitions(std::shared_ptr<std::vector<PartitionMetaInfo>> partition_metainfos) {
  auto table_schemas = config_manager_->getTableSchemas();
  if (!table_schemas) {
    return;
  }
  for (auto& table : *table_schemas) {
    uint32_t partition_number = table.second->getPartitionNumber();
    auto database_name = table.second->getDatabaseName();
    auto table_name = table.second->getTableName();
    for (int id = 0; id < partition_number; ++id) {
      auto partition = std::make_shared<Partition>(database_name, table_name, id);
      auto partition_handler = getPartitionHandler(partition);
      if (!partition_handler.hasValue()) {
        if (database_meta_info_->deleteVersion(partition)) {
          PartitionMetaInfo meta_info;
          meta_info.setPartitionId(id);
          meta_info.setDatabaseName(database_name);
          meta_info.setTableName(table_name);
          meta_info.setHash(partition->getPartitionHash());
          partition_metainfos->emplace_back(meta_info);
        }
      }
    }
  }
}

void DatabaseManager::updateConfigManually(const std::unordered_map<std::string, std::string>& configs) {
  config_manager_->setUseManuallySetConfig(true);
  config_manager_->updateConfig(configs, group_name_, node_id_, false);
}

uint64_t DatabaseManager::getTableHash(const std::string& database_name, const std::string& table_name) {
  uint64_t key = CityHash64WithSeed(database_name.data(), database_name.size(), 0);
  return CityHash64WithSeed(table_name.data(), table_name.size(), key);
}

int64_t DatabaseManager::getNodeHash() { return CityHash64WithSeed(group_name_.data(), group_name_.size(), node_id_); }

void DatabaseManager::setShardMetrics() {
  auto metrics = metrics::Metrics::getInstance();
  std::weak_ptr<DatabaseManager> weak_data_manager = shared_from_this();
  metrics->buildBatchGauges(LASER_METRICS_MODULE_NAME_FOR_SHARD, LASER_METRICS_METRIC_NAME_FOR_SHARD,
                            METRIC_ROCKSDB_STAT_INTERVAL, [weak_data_manager]() {
                              std::vector<metrics::TagMetric> tag_metrics;
                              auto data_manager = weak_data_manager.lock();
                              if (!data_manager) {
                                return tag_metrics;
                              }
                              return data_manager->getShardMetricData();
                            });
}

std::vector<metrics::TagMetric> DatabaseManager::getShardMetricData() {
  std::unordered_map<uint32_t, std::vector<std::shared_ptr<PartitionHandler>>> shard_partitions;
  std::unordered_map<int64_t, std::shared_ptr<PartitionHandler>> partitions;
  partition_handlers_.withRLock([&partitions](auto& handlers) { partitions = handlers; });
  for (auto& partition : partitions) {
    uint32_t shard_id = partition.second->getPartition()->getShardId();
    auto partition_list = shard_partitions.find(shard_id);
    if (partition_list == shard_partitions.end()) {
      shard_partitions[shard_id] = {};
    }
    shard_partitions[shard_id].push_back(partition.second);
  }

  std::vector<std::string> keys;
  PartitionHandler::getPropertyKeys(&keys);
  std::vector<metrics::TagMetric> tag_metrics;
  for (auto& partition_list : shard_partitions) {
    metrics::TagKey shardKey(LASER_METRICS_TAG_SHARD_ID, folly::to<std::string>(partition_list.first));
    metrics::TagKey nodeKey(LASER_METRICS_TAG_NODE_ID, folly::to<std::string>(node_id_));
    metrics::TagKey groupKey(LASER_METRICS_TAG_GROUP_ID, group_name_);
    std::vector<metrics::TagKey> tag_keys({shardKey, nodeKey});
    for (auto& key : keys) {
      auto temp_tag_keys = tag_keys;
      metrics::TagKey propertyKey(LASER_METRICS_TAG_PROPERTY, key);
      temp_tag_keys.push_back(propertyKey);
      double value = 0.0;
      for (auto& partition : partition_list.second) {
        value += static_cast<double>(partition->getProperty(key));
      }
      metrics::TagMetric metric(temp_tag_keys, value);
      tag_metrics.push_back(metric);
    }
  }

  return tag_metrics;
}

void DatabaseManager::setTableMetrics(const std::string& database_name, const std::string& table_name) {
  std::vector<std::string> keys;
  PartitionHandler::getPropertyKeys(&keys);
  std::vector<DBRole> roles({DBRole::LEADER, DBRole::FOLLOWER});

  std::weak_ptr<DatabaseManager> weak_data_manager = shared_from_this();
  auto metrics = metrics::Metrics::getInstance();
  for (auto& role : roles) {
    std::unordered_map<std::string, std::string> tags = {
        {LASER_METRICS_TAG_DB_NAME, database_name},
        {LASER_METRICS_TAG_TABLE_NAME, table_name},
        {LASER_METRICS_TAG_ROLE_NAME, toStringDBRole(role)},
    };
    for (auto& key : keys) {
      metrics->buildGauges(
          LASER_METRICS_MODULE_NAME_FOR_ROCKSDB_TABLE, key, METRIC_ROCKSDB_STAT_INTERVAL,
          [database_name, table_name, key, role, weak_data_manager]() {
            auto data_manager = weak_data_manager.lock();
            if (!data_manager) {
              return 0.0;
            }
            return data_manager->getMetricData(database_name, table_name, role, key);
          },
          tags);
    }

    metrics->buildGauges(
        LASER_METRICS_MODULE_NAME_FOR_ROCKSDB_TABLE, LASER_METRIC_NAME_FOR_PARTITION_NUM, METRIC_ROCKSDB_STAT_INTERVAL,
        [database_name, table_name, role, weak_data_manager]() {
          auto data_manager = weak_data_manager.lock();
          if (!data_manager) {
            return 0.0;
          }
          return data_manager->getTableMountedPartitionNumberByRole(database_name, table_name, role);
        },
        tags);
  }
}

double DatabaseManager::getMetricData(const std::string& database_name, const std::string& table_name,
                                      const DBRole& role, const std::string& key) {
  auto table = config_manager_->getTableSchema(database_name, table_name);
  if (!table) {
    VLOG(5) << "Not exists database:" << database_name << " table:" << table_name;
    return 0.0;
  }

  double value = 0.0;
  uint32_t partition_number = table.value()->getPartitionNumber();
  for (int i = 0; i < partition_number; i++) {
    std::shared_ptr<Partition> partition = std::make_shared<Partition>(database_name, table_name, i);
    auto partition_handler = getPartitionHandler(partition);
    if (!partition_handler.hasValue()) {
      continue;
    }
    if (partition_handler.value()->getPartition()->getRole() != role) {
      continue;
    }

    value += static_cast<double>(partition_handler.value()->getProperty(key));
  }

  return value;
}

double DatabaseManager::getTableMountedPartitionNumberByRole(const std::string& database_name,
                                                             const std::string& table_name, const DBRole& role) {
  auto table = config_manager_->getTableSchema(database_name, table_name);
  if (!table) {
    VLOG(5) << "Not exists database:" << database_name << " table:" << table_name;
    return 0.0;
  }

  double value = 0.0;
  uint32_t partition_number = table.value()->getPartitionNumber();
  for (int i = 0; i < partition_number; i++) {
    std::shared_ptr<Partition> partition = std::make_shared<Partition>(database_name, table_name, i);
    auto partition_handler = getPartitionHandler(partition);
    if (!partition_handler.hasValue()) {
      continue;
    }
    if (partition_handler.value()->getPartition()->getRole() != role) {
      continue;
    }
    value++;
  }

  return value;
}

void DatabaseManager::delaySetAvailable() {
  std::weak_ptr<DatabaseManager> weak_db_manager = shared_from_this();
  timer_thread_->getEventBase()->runInEventBaseThread([weak_db_manager] {
    auto db_manager = weak_db_manager.lock();
    if (!db_manager) {
      return;
    }
    db_manager->timer_thread_->getEventBase()->runAfterDelay(
        [weak_db_manager] {
          auto db_manager = weak_db_manager.lock();
          if (!db_manager) {
            return;
          }
          if (db_manager->has_api_server_) {
            db_manager->config_manager_->getRouter()->setStatus(db_manager->api_server_.getServiceName(),
                                                                service_router::ServerStatus::AVAILABLE);
            LOG(INFO) << "Set server status AVAILABLE, delay time: " << FLAGS_delay_set_available_seconds
                      << " seconds.";
          } else {
            db_manager->delaySetAvailable();
            FB_LOG_EVERY_MS(WARNING, 2000)
                << "Restart a delay set available schedule. Has api server is: " << db_manager->has_api_server_
                << ", delay time: " << FLAGS_delay_set_available_seconds << " seconds.";
          }
        },
        FLAGS_delay_set_available_seconds * 1000);
  });
}

}  // namespace laser
