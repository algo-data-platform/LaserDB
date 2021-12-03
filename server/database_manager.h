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

#pragma once

#include "folly/Synchronized.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "folly/io/async/AsyncTimeout.h"

#include "common/laser/partition.h"
#include "common/laser/rocksdb_config_factory.h"
#include "common/metrics/metrics.h"
#include "common/service_router/router.h"

#include "database_meta_info.h"
#include "datapath_manager.h"
#include "engine/replicator_manager.h"
#include "engine/wdt_replicator.h"
#include "partition_handler.h"
#include "table_monitor.h"

namespace laser {

class DatabaseManagerTimerThread : public folly::ScopedEventBaseThread {
 public:
  DatabaseManagerTimerThread() : folly::ScopedEventBaseThread("DatabaseTimerThread") {}
  ~DatabaseManagerTimerThread() { VLOG(5) << "Database manager timer thread delete."; }
};

// 一定要在 config manager 之前初始化, 进行设置订阅配置变更回调
class DatabaseManager : public std::enable_shared_from_this<DatabaseManager> {
 public:
  DatabaseManager(std::shared_ptr<ConfigManager> config_manager, const std::string& group_name, uint32_t node_id,
                  const std::string& node_dc)
      : config_manager_(config_manager), group_name_(group_name), node_id_(node_id), node_dc_(node_dc) {}
  virtual ~DatabaseManager();
  virtual void init(uint32_t loader_thread_nums, const std::string& replicator_service_name,
                    const std::string& replicator_host, uint32_t replicator_port);
  // 服务启动是同步 thrift server状态信息
  virtual inline void setServiceServer(const service_router::Server& server) {
    api_server_ = server;
    // 等待第一次同步配置信息, 进行第一次 server 信息同步
    semaphore_.wait();
    has_api_server_ = true;
    updateServerShard();
  }

  virtual inline const std::string& getGroupName() const { return group_name_; }
  virtual inline uint32_t getNodeId() const { return node_id_; }
  virtual void updatePartitions(const PartitionPtrSet& mount_partitions, const PartitionPtrSet& unmount_partitions);
  virtual folly::Optional<std::weak_ptr<RocksDbEngine>> getDatabaseHandler(const std::shared_ptr<Partition>& partition);
  virtual std::shared_ptr<RocksDbConfigFactory> getRocksdbConfigFactory() { return rocksdb_config_factory_; }
  virtual void triggerBase(const std::string& database_name, const std::string& table_name, const std::string& version);
  virtual void triggerDelta(const std::string& database_name, const std::string& table_name,
                            const std::string& base_version, const std::vector<std::string>& delta_versions);
  virtual void triggerBaseDataReplication(const std::string& database_name, const std::string& table_name);
  virtual void getTableMetaInfo(TableMetaInfo* table_info, const std::string& database_name,
                                const std::string& table_name);
  virtual void setUnavailableShards(const std::vector<uint32_t> shard_ids);
  virtual void getShardMetaInfo(std::vector<std::shared_ptr<ShardMetaInfo>>* shards);
  virtual void monitorSwitch(bool switch_flag);
  virtual void cleanUnusedPartitions(std::shared_ptr<std::vector<PartitionMetaInfo>> partition_metainfos);
  virtual void updateConfigManually(const std::unordered_map<std::string, std::string>& configs);
  virtual const std::string getReplicatorServiceName() { return replicator_service_name_; }
  virtual const std::string getReplicatorHost() { return replicator_host_; }
  virtual int64_t getNodeHash();
  virtual const std::string getNodeDc() { return node_dc_; }

 private:
  std::shared_ptr<ConfigManager> config_manager_;
  service_router::Server api_server_;
  std::string group_name_;
  uint32_t node_id_;
  std::string node_dc_;
  std::string replicator_service_name_;
  std::string replicator_host_;
  folly::SaturatingSemaphore<true> semaphore_;
  folly::SaturatingSemaphore<true> rocksdb_config_semaphore_;
  std::atomic<bool> has_api_server_{false};
  std::atomic_flag meta_info_inited_ = ATOMIC_FLAG_INIT;
  std::shared_ptr<PartitionManager> partition_manager_;
  std::shared_ptr<RocksDbConfigFactory> rocksdb_config_factory_;
  std::shared_ptr<DatabaseMetaInfo> database_meta_info_;
  std::shared_ptr<ReplicatorManager> replicator_manager_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> loader_thread_pool_;
  std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor_manager_;
  std::shared_ptr<WdtReplicatorManager> wdt_manager_;
  std::unique_ptr<DatabaseManagerTimerThread> timer_thread_;
  std::unique_ptr<metrics::MetricsThread> self_hold_metrics_thread_;
  std::unique_ptr<folly::AsyncTimeout> expire_timer_;
  folly::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<TableMonitor>>> table_monitors_;
  folly::Synchronized<std::unordered_map<int64_t, std::shared_ptr<PartitionHandler>>> partition_handlers_;
  folly::Synchronized<std::vector<uint32_t>> unavailable_shards_;

  virtual std::shared_ptr<folly::CPUThreadPoolExecutor> createLoaderThreadPool(uint32_t thread_nums);
  virtual std::shared_ptr<PartitionManager> createPartitionManager();
  virtual std::shared_ptr<hdfs::HdfsMonitorManager> createHdfsMonitorManager();
  virtual std::shared_ptr<RocksDbConfigFactory> createRocksDbConfigFactory();
  virtual std::shared_ptr<DatabaseMetaInfo> createDatabaseMetaInfo();
  virtual std::shared_ptr<ReplicatorManager> createReplicatorManager();
  virtual std::shared_ptr<WdtReplicatorManager> createWdtReplicatorManager(uint32_t thread_nums);
  virtual uint64_t getTableHash(const std::string& database_name, const std::string& table_name);
  virtual std::shared_ptr<TableMonitor> getOrCreateTableMonitor(const std::shared_ptr<Partition>& partition);
  virtual std::shared_ptr<TableMonitor> getOrCreateTableMonitor(const std::string& database_name,
                                                                const std::string& table_name);
  virtual std::shared_ptr<PartitionHandler> getOrCreatePartitionHandler(const std::shared_ptr<Partition>& partition);
  virtual folly::Optional<std::shared_ptr<PartitionHandler>> getPartitionHandler(
      const std::shared_ptr<Partition>& partition);
  void removePartitionHandler(const std::shared_ptr<Partition>& partition);

  virtual void updateServerShard();
  virtual int64_t getShardUniqueId(uint32_t shard_id, const DBRole& role);
  virtual void setTableMetrics(const std::string& database_name, const std::string& table_name);
  virtual void setShardMetrics();
  virtual std::vector<metrics::TagMetric> getShardMetricData();
  virtual double getMetricData(const std::string& database_name, const std::string& table_name, const DBRole& role,
                               const std::string& key);
  virtual double getTableMountedPartitionNumberByRole(const std::string& database_name, const std::string& table_name,
                                                      const DBRole& role);
  virtual void delaySetAvailable();
};

}  // namespace laser
