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
 */

#pragma once

#include "folly/SpinLock.h"
#include "folly/Synchronized.h"
#include "folly/ProducerConsumerQueue.h"
#include "folly/io/async/AsyncTimeout.h"

#include "common/laser/partition.h"
#include "common/laser/versioned_options.h"
#include "engine/rocksdb.h"
#include "database_meta_info.h"
#include "engine/replicator_manager.h"

namespace laser {

enum class PartitionLoadStatus {
  BASE_LOADED,
  BASE_LOADING,
  DELTA_LOADED,
  DELTA_LOADING,
};

enum class PartitionLoadType {
  BASE_LOAD,
  DELTA_LOAD,
};

class PartitionLoadInfo {
 public:
  PartitionLoadInfo() = default;
  ~PartitionLoadInfo() = default;
  PartitionLoadInfo(const PartitionLoadType& type, const std::string& base_version,
                    const std::vector<std::string>& delta_versions)
      : type_(type), base_version_(base_version), delta_versions_(delta_versions) {}
  PartitionLoadInfo(const PartitionLoadType& type, const std::string& base_version)
      : PartitionLoadInfo(type, base_version, std::vector<std::string>()) {}
  const std::string& getBaseVersion() const { return base_version_; }
  const std::vector<std::string>& getDeltaVersions() const { return delta_versions_; }
  const PartitionLoadType& getType() const { return type_; }

 private:
  PartitionLoadType type_;
  std::string base_version_;
  std::vector<std::string> delta_versions_;
};

class DatabaseManager;
class PartitionHandler : public std::enable_shared_from_this<PartitionHandler> {
 public:
  PartitionHandler(std::shared_ptr<Partition> partition, DatabaseManager* database_manager,
                   DatabaseMetaInfo* database_meta_info, ReplicatorManager* replicator_manager,
                   std::shared_ptr<WdtReplicatorManager> wdt_manager, folly::EventBase* timer_evb,
                   std::shared_ptr<RocksDbEngineOptions> engine_options,
                   folly::EventBase* self_hold_metrics_evb = nullptr);
  virtual ~PartitionHandler();

  virtual bool init();
  virtual void loadBaseData(const std::string& version);
  virtual void loadDeltaData(const std::string& base_version, const std::vector<std::string>& delta_versions);
  virtual std::weak_ptr<RocksDbEngine> getDatabase() { return db_; }
  virtual const std::shared_ptr<Partition> getPartition() { return partition_; }
  virtual void setPartition(std::shared_ptr<Partition> partition) {
    folly::SpinLockGuard g(spinlock_);
    if (partition->getRole() != partition_->getRole() && db_) {
      auto weak_db = db_->getReplicationDB();
      auto db = weak_db.lock();
      if (db) {
        db->changeRole(partition->getRole());
      }
    }
    partition_ = partition;
  }
  virtual void getPartitionMetaInfo(PartitionMetaInfo* info);
  virtual uint64_t getProperty(const std::string& key);
  static void getPropertyKeys(std::vector<std::string>* keys);
  virtual void forceBaseDataReplication();

 private:
  static uint32_t PARTITION_HANDLER_BASE_MAX_QUEUE_SIZE;
  std::shared_ptr<Partition> partition_;
  DatabaseManager* database_manager_;
  DatabaseMetaInfo* database_meta_info_;
  ReplicatorManager* replicator_manager_;
  std::shared_ptr<WdtReplicatorManager> wdt_manager_;
  std::string base_version_;
  std::shared_ptr<RocksDbEngine> db_{nullptr};
  folly::SpinLock spinlock_;
  std::vector<std::string> delta_versions_;
  PartitionLoadStatus status_{PartitionLoadStatus::BASE_LOADED};

  std::shared_ptr<folly::ProducerConsumerQueue<PartitionLoadInfo>> load_queue_;
  folly::EventBase* timer_evb_;
  folly::EventBase* self_hold_metrics_evb_;
  std::shared_ptr<RocksDbEngineOptions> engine_options_;
  VersionedOptions versioned_options_;
  std::unique_ptr<folly::AsyncTimeout> timeout_;

  // 如果执行 base data wdt 方式同步将置为 true, 由于 wdt 同步时 增量同步还在进行，此时
  // 会不断出发 wdt 全量同步，该标志是为了仅触发一次
  std::atomic_flag base_data_wdt_replicating_ = ATOMIC_FLAG_INIT;
  std::atomic_flag has_delay_wdt_replication_ = ATOMIC_FLAG_INIT;

 protected:
  virtual void triggerLoadData();
  virtual bool ingestBaseData(std::shared_ptr<RocksDbEngine>* db, const std::string& version);
  virtual bool ingestDeltaData(const std::string& version);
  virtual bool canLoadCheck();
  virtual bool createRocksDbEngine(std::shared_ptr<RocksDbEngine>* result_db, const std::string& version);
  virtual void exchangeBaseVersion(std::shared_ptr<RocksDbEngine> db, const std::string& version,
                                   const std::vector<std::string>& delta_versions = {});
  virtual std::shared_ptr<folly::ProducerConsumerQueue<PartitionLoadInfo>> createLoaderQueue();
  virtual void setUpdateVersionCallback(std::shared_ptr<laser::ReplicationDB> replication_db);
  virtual bool removeExistingDataDir(std::string* data_dir, const std::string& version);
  virtual void baseDataReplicate(const std::string& db_hash, const std::string& version);
  virtual void delayBaseDataReplicate(const std::string& db_hash, const std::string& version);
  virtual void notifyWdtSender(const std::string& connect_url, const std::string& db_hash, const std::string& version,
                               std::shared_ptr<laser::WdtReplicator> wdt);
  virtual bool updateRocksdbOptions();
  virtual void syncDestoryRocksDbEngine(std::shared_ptr<RocksDbEngine>&& db);
  virtual bool loadRocksDbEngine(const std::string& version, const std::vector<std::string>& delta_versions = {});
  virtual bool reloadRocksDbEngine(const std::string& version, const std::vector<std::string>& delta_versions = {});
  virtual uint64_t getPartitionSize();
  virtual uint64_t getPartitionReadKps();
  virtual uint64_t getPartitionWriteKps();
  virtual uint64_t getPartitionReadBytes();
  virtual uint64_t getPartitionWriteBytes();
};

}  // namespace laser
