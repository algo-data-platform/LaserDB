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

#include "boost/filesystem.hpp"

#include "common/laser/if/gen-cpp2/ReplicatorAsyncClient.h"
#include "common/laser/status.h"

#include "database_manager.h"
#include "partition_handler.h"

namespace laser {

DEFINE_int32(wdt_replicator_error_delay_ms, 5000, "Wdt replicator delay ms when execute error");
DEFINE_int32(wdt_replicator_max_server_wait_time_ms, 500, "Wdt replicator request timeout");
DEFINE_int32(rocksdb_options_check_interval_ms, 5000, "Rocksdb option check interval");
DEFINE_int32(rocksdb_engine_destory_wait_interval_ms, 10, "Rocksdb engine destory wait interval");
DEFINE_int32(finish_rocksdb_processing_operation_time_ms, 5,
             "Time wait for rocksdb finishing processing operations before closing");

uint32_t PartitionHandler::PARTITION_HANDLER_BASE_MAX_QUEUE_SIZE = 10;
constexpr static char PARTITION_SIZE_PROPERTY[] = "rocksdb.live-sst-files-size";

PartitionHandler::PartitionHandler(std::shared_ptr<Partition> partition, DatabaseManager* database_manager,
                                   DatabaseMetaInfo* database_meta_info, ReplicatorManager* replicator_manager,
                                   std::shared_ptr<WdtReplicatorManager> wdt_manager, folly::EventBase* timer_evb,
                                   std::shared_ptr<RocksDbEngineOptions> engine_options,
                                   folly::EventBase* self_hold_metrics_evb)
    : partition_(partition),
      database_manager_(database_manager),
      database_meta_info_(database_meta_info),
      replicator_manager_(replicator_manager),
      wdt_manager_(wdt_manager),
      timer_evb_(timer_evb),
      self_hold_metrics_evb_(self_hold_metrics_evb),
      engine_options_(engine_options) {}

std::shared_ptr<folly::ProducerConsumerQueue<PartitionLoadInfo>> PartitionHandler::createLoaderQueue() {
  return std::make_shared<folly::ProducerConsumerQueue<PartitionLoadInfo>>(PARTITION_HANDLER_BASE_MAX_QUEUE_SIZE);
}

PartitionHandler::~PartitionHandler() {
  if (timeout_) {
    timer_evb_->runInEventBaseThreadAndWait([this]() { timeout_->cancelTimeout(); });
  }
  LOG(INFO) << "Partition " << *partition_ << " db handler delete.";
}

bool PartitionHandler::init() {
  load_queue_ = createLoaderQueue();
  std::weak_ptr<PartitionHandler> handler = shared_from_this();
  timeout_ = folly::AsyncTimeout::make(*timer_evb_, [handler]() noexcept {
    auto partition_handler = handler.lock();
    if (!partition_handler) {
      return;
    }
    partition_handler->updateRocksdbOptions();
  });

  return updateRocksdbOptions();
}

void PartitionHandler::exchangeBaseVersion(std::shared_ptr<RocksDbEngine> db, const std::string& version,
                                           const std::vector<std::string>& delta_versions) {
  db_ = std::move(db);
  base_version_ = version;
  delta_versions_ = delta_versions;
  VLOG(5) << "Add db to replicator manager, partition:" << *partition_ << " version:" << version;
  replicator_manager_->addDB(partition_->getRole(), partition_->getShardId(), partition_->getPartitionHash(), version,
                             wdt_manager_, db_->getReplicationDB(), partition_->getDc());
  database_meta_info_->updateVersion(partition_, version);
  database_meta_info_->updateDeltaVersions(partition_, delta_versions_);
}

void PartitionHandler::setUpdateVersionCallback(std::shared_ptr<laser::ReplicationDB> replication_db) {
  replication_db->setUpdateVersionCallback([this](int64_t db_hash, const std::string& version) {
    if (partition_->getRole() != DBRole::FOLLOWER) {
      return;
    }

    if (base_data_wdt_replicating_.test_and_set()) {
      VLOG(5) << "Current db " << db_hash << " in replication base data, version:" << version;
      return;
    }

    baseDataReplicate(folly::to<std::string>(db_hash), version);
  });
}

bool PartitionHandler::removeExistingDataDir(std::string* data_dir, const std::string& version) {
  std::string replicating_data_dir = "";
  std::string base_version = "";
  {
    folly::SpinLockGuard g(spinlock_);
    base_version = base_version_;
  }
  if (version == base_version) {
    replicating_data_dir = common::pathJoin(
        DataPathManager::getDatabaseDataReplicatingDir(database_manager_, partition_, version), "data");
    std::string deleting_data_dir = DataPathManager::getDatabaseDataDeletingDir(database_manager_, partition_, version);
    boost::filesystem::path bdata(deleting_data_dir);
    if (boost::filesystem::exists(bdata)) {
      try {
        boost::filesystem::remove_all(bdata);
      } catch (...) {
        LOG(ERROR) << "Remove db data deleting dir fail, partition:" << *partition_ << " path:" << bdata.string();
        return false;
      }
    }
  } else {
    replicating_data_dir =
        common::pathJoin(DataPathManager::getDatabaseDataDir(database_manager_, partition_, version), "data");
  }

  boost::filesystem::path bdata(replicating_data_dir);
  if (boost::filesystem::exists(bdata)) {
    try {
      boost::filesystem::remove_all(bdata);
    } catch (...) {
      LOG(ERROR) << "Remove db data fail, partition:" << *partition_ << " path:" << bdata.string();
      return false;
    }
  }
  *data_dir = replicating_data_dir;
  return true;
}

void PartitionHandler::baseDataReplicate(const std::string& db_hash, const std::string& version) {
  bool ret = false;
  std::shared_ptr<laser::WdtReplicator> wdt = nullptr;
  std::string connect_url;
  std::string data_dir;
  do {
    ret = removeExistingDataDir(&data_dir, version);
    if (!ret) {
      LOG(ERROR) << "Remove existing data dir failed!";
      break;
    }
    wdt = std::make_shared<laser::WdtReplicator>(wdt_manager_, db_hash, db_hash, FLAGS_wdt_replicator_abort_timeout_ms);
    std::weak_ptr<PartitionHandler> handler = shared_from_this();
    ret = wdt->receiver(&connect_url, database_manager_->getReplicatorHost(), data_dir,
                        [handler, db_hash, version](auto& name_space, auto& ident, auto error) {
                          auto partition_handler = handler.lock();
                          if (!partition_handler) {
                            return;
                          }
                          bool base_data_replicate_success = false;
                          do {
                            if (error != facebook::wdt::ErrorCode::OK) {
                              LOG(ERROR) << "Transfer receiver complete, db_hash:" << db_hash << " version:" << version
                                         << " err:" << facebook::wdt::errorCodeToStr(error);
                              break;
                            }

                            std::string deleting_data_dir = "";
                            bool base_data_replicate_on_same_version = false;
                            {
                              folly::SpinLockGuard g(partition_handler->spinlock_);
                              if (version == partition_handler->base_version_) {
                                base_data_replicate_on_same_version = true;
                                if (partition_handler->db_) {
                                  partition_handler->syncDestoryRocksDbEngine(std::move(partition_handler->db_));
                                }
                                deleting_data_dir = DataPathManager::getDatabaseDataDeletingDir(
                                    partition_handler->database_manager_, partition_handler->partition_, version);
                                std::string data_dir = DataPathManager::getDatabaseDataDir(
                                    partition_handler->database_manager_, partition_handler->partition_, version);
                                std::string replicating_data_dir = DataPathManager::getDatabaseDataReplicatingDir(
                                    partition_handler->database_manager_, partition_handler->partition_, version);
                                boost::system::error_code ec;
                                if (boost::filesystem::exists(data_dir)) {
                                  boost::filesystem::rename(data_dir, deleting_data_dir, ec);
                                  if (ec) {
                                    LOG(ERROR) << "Rename from data_dir: " << data_dir << " to " << deleting_data_dir
                                               << " failed! ErrorCode: " << ec.value() << ", Message: " << ec.message();
                                    break;
                                  }
                                }

                                boost::filesystem::rename(replicating_data_dir, data_dir, ec);
                                if (ec) {
                                  LOG(ERROR)
                                      << "Rename from replicating_dir: " << replicating_data_dir << " to " << data_dir
                                      << " failed! ErrorCode: " << ec.value() << ", Message: " << ec.message();
                                  break;
                                }
                              }
                              partition_handler->loadRocksDbEngine(version);
                              partition_handler->base_data_wdt_replicating_.clear();
                            }
                            if (base_data_replicate_on_same_version) {
                              boost::system::error_code ec;
                              boost::filesystem::remove_all(deleting_data_dir, ec);
                              if (ec) {
                                LOG(ERROR) << "Remove deleting_data_dir: " << deleting_data_dir
                                           << " failed! ErrorCode: " << ec.value() << ", Message: " << ec.message();
                              }
                            }
                            VLOG(5) << "Follower db version has update, partition:" << *(partition_handler->partition_)
                                    << " db_hash:" << db_hash << " version:" << version;
                            base_data_replicate_success = true;
                          } while (false);

                          if (!base_data_replicate_success) {
                            // 如果本次数据同步失败，设置延迟同步
                            partition_handler->base_data_wdt_replicating_.clear();
                            if (!partition_handler->has_delay_wdt_replication_.test_and_set()) {
                              LOG(INFO) << "Base data replication failed, schedule a delay base data replication!";
                              partition_handler->delayBaseDataReplicate(db_hash, version);
                            }
                          }
                        });
    if (!ret) {
      LOG(ERROR) << "Receive data via wdt failed!";
      break;
    }
  } while (false);

  if (!ret) {
    if (!has_delay_wdt_replication_.test_and_set()) {
      LOG(INFO) << "Base data replication failed, schedule a delay base data replication!";
      delayBaseDataReplicate(db_hash, version);
    }
  } else {
    notifyWdtSender(connect_url, db_hash, version, wdt);
  }
}

void PartitionHandler::notifyWdtSender(const std::string& connect_url, const std::string& db_hash,
                                       const std::string& version, std::shared_ptr<laser::WdtReplicator> wdt) {
  ReplicateWdtRequest request;
  request.wdt_url = connect_url;
  request.db_hash = folly::to<int64_t>(db_hash);
  request.version = version;
  request.node_hash = database_manager_->getNodeHash();
  apache::thrift::RpcOptions rpc_options;
  rpc_options.setTimeout(
      std::chrono::milliseconds(FLAGS_wdt_replicator_max_server_wait_time_ms + FLAGS_wdt_replicator_abort_timeout_ms));

  std::weak_ptr<PartitionHandler> handler = shared_from_this();

  auto process_request = [handler, wdt](folly::Try<ReplicateWdtResponse>&& try_response) {
    auto partition_handler = handler.lock();
    if (partition_handler == nullptr) {
      return;
    }
    if (try_response.hasException()) {
      try {
        try_response.exception().throw_exception();
      } catch (const LaserException& ex) {
        LOG(ERROR) << "LaserException: " << ex.get_message();
      } catch (const std::exception& ex) {
        LOG(ERROR) << "std::exception: " << ex.what();
      }
      wdt->abort();
    }
  };

  auto send_request = [&rpc_options, &request, this, handler,
                       process_request = std::move(process_request)](auto client) {
    client->future_replicateWdt(rpc_options, request).then(std::move(process_request));
  };

  service_router::ClientOption option;
  option.setServiceName(database_manager_->getReplicatorServiceName());
  option.setDc(partition_->getDc());
  option.setProtocol(service_router::ServerProtocol::THRIFT);
  option.setShardId(partition_->getSrcShardId());
  bool ret = service_router::thriftServiceCall<ReplicatorAsyncClient>(option, std::move(send_request));
  if (!ret) {
    wdt->abort();
  }
}

void PartitionHandler::delayBaseDataReplicate(const std::string& db_hash, const std::string& version) {
  std::weak_ptr<PartitionHandler> handler = shared_from_this();
  timer_evb_->runInEventBaseThread([handler = std::move(handler), db_hash, version]() {
    auto partition_handler = handler.lock();
    if (!partition_handler) {
      return;
    }
    partition_handler->timer_evb_->runAfterDelay(
        [handler = std::move(handler), db_hash, version] {
          auto partition_handler = handler.lock();
          if (partition_handler) {
            partition_handler->has_delay_wdt_replication_.clear();
            partition_handler->baseDataReplicate(db_hash, version);
          }
        },
        FLAGS_wdt_replicator_error_delay_ms);
  });
}

bool PartitionHandler::createRocksDbEngine(std::shared_ptr<RocksDbEngine>* result_db, const std::string& version) {
  auto options = versioned_options_.getOptions();
  std::string data_dir = DataPathManager::getDatabaseDataDir(database_manager_, partition_, version);
  auto replication_db = std::make_shared<laser::ReplicationDB>(data_dir, options);
  if (self_hold_metrics_evb_) {
    replication_db->init(self_hold_metrics_evb_);
  }
  setUpdateVersionCallback(replication_db);

  auto db = std::make_shared<laser::RocksDbEngine>(replication_db, engine_options_);
  if (!db->open()) {
    LOG(INFO) << "Open empty db fail, "
              << " partition:" << *partition_ << " version:" << version;
    return false;
  }

  *result_db = db;
  return true;
}

bool PartitionHandler::ingestBaseData(std::shared_ptr<RocksDbEngine>* result_db, const std::string& version) {
  std::string source_data_file = DataPathManager::getSourceBaseDataFile(partition_, version);
  std::string data_dir = DataPathManager::getDatabaseDataDir(database_manager_, partition_, version);
  boost::filesystem::path bsource_data_file(source_data_file);
  if (!boost::filesystem::exists(bsource_data_file)) {
    LOG(INFO) << "Load base data not exists, source data file:" << source_data_file << " partition:" << *partition_
              << " version:" << version;
    return false;
  }

  boost::filesystem::path bdata_dir(data_dir);
  bool need_ingest = true;
  if (boost::filesystem::exists(bdata_dir)) {
    LOG(INFO) << "Current version data has exists, not need ingest data"
              << " partition:" << *partition_ << " version:" << version;
    need_ingest = false;
  }

  if (!createRocksDbEngine(result_db, version)) {
    return false;
  }

  if (need_ingest) {
    if (Status::OK != (*result_db)->ingestBaseSst(source_data_file)) {
      LOG(INFO) << "Ingest sst fail, "
                << " partition:" << *partition_ << " version:" << version;
      return false;
    }
  }
  LOG(INFO) << "Partition:" << *partition_ << " version:" << version << " load base data success.";
  return true;
}

bool PartitionHandler::canLoadCheck() {
  return (status_ == PartitionLoadStatus::BASE_LOADED || status_ == PartitionLoadStatus::DELTA_LOADED);
}

void PartitionHandler::loadBaseData(const std::string& version) {
  {  // start modify status
    spinlock_.lock();
    if (base_version_ == version) {
      LOG(INFO) << "Partition:" << *partition_ << " load base data version: " << version
                << " current service version:" << base_version_;
      spinlock_.unlock();
      triggerLoadData();
      return;
    }

    if (!canLoadCheck()) {
      PartitionLoadInfo info(PartitionLoadType::BASE_LOAD, version);
      if (!load_queue_->write(info)) {
        LOG(ERROR) << "Base load data temp store queue is full, queue size:" << PARTITION_HANDLER_BASE_MAX_QUEUE_SIZE
                   << " partition:" << *partition_ << " load base version:" << version;
      }
      VLOG(3) << "Current loading other patch";
      spinlock_.unlock();
      return;
    }

    status_ = PartitionLoadStatus::BASE_LOADING;
    spinlock_.unlock();
  }

  // ingest sst data
  std::shared_ptr<RocksDbEngine> db;
  bool ingest_ret = ingestBaseData(&db, version);

  {  // end modify status
    folly::SpinLockGuard g(spinlock_);
    if (ingest_ret) {
      exchangeBaseVersion(db, version);
    }
    status_ = PartitionLoadStatus::BASE_LOADED;
  }

  triggerLoadData();
}

void PartitionHandler::triggerLoadData() {
  PartitionLoadInfo info;
  VLOG(3) << "Trigger load data  queue base data, partition:" << *partition_
          << " queue size: " << load_queue_->sizeGuess();
  if (!load_queue_->read(info)) {
    return;
  }

  if (info.getType() == PartitionLoadType::BASE_LOAD) {
    VLOG(3) << "Consumer loader queue base data, partition:" << *partition_ << " version:" << info.getBaseVersion();
    loadBaseData(info.getBaseVersion());
  } else {
    loadDeltaData(info.getBaseVersion(), info.getDeltaVersions());
  }
}

bool PartitionHandler::ingestDeltaData(const std::string& version) {
  VLOG(2) << "Start load delta data, partition:" << *partition_ << " base version:" << base_version_
          << " delta version:" << version;
  std::string temp_db_path = DataPathManager::getDeltaLoadDataTempDbPath(partition_, base_version_, version);
  std::string delta_sst_path = DataPathManager::getSourceDeltaDataFile(partition_, base_version_, version);
  boost::filesystem::path bsource_data_file(delta_sst_path);
  if (!boost::filesystem::exists(bsource_data_file)) {
    LOG(INFO) << "Load delta data not exists, source data file:" << delta_sst_path << " partition:" << *partition_
              << " base version:" << base_version_ << " version:" << version;
    return false;
  }

  Status status;
  {
    folly::SpinLockGuard g(spinlock_);
    if (db_) {
      status = db_->ingestDeltaSst(delta_sst_path, temp_db_path);
    }
  }
  boost::filesystem::path bdata_dir(temp_db_path);
  if (boost::filesystem::exists(bdata_dir)) {
    boost::filesystem::remove_all(bdata_dir);
  }
  if (status != Status::OK) {
    LOG(INFO) << "Load delta data fail, source data file:" << delta_sst_path << " partition:" << *partition_
              << " base version:" << base_version_ << " version:" << version << " status:" << status;
    return false;
  }

  return true;
}

void PartitionHandler::loadDeltaData(const std::string& base_version, const std::vector<std::string>& delta_versions) {
  VLOG(2) << "Start load delta data, partition:" << *partition_ << " base version:" << base_version_;
  {  // start modify status
    spinlock_.lock();
    if (!canLoadCheck()) {
      PartitionLoadInfo info(PartitionLoadType::DELTA_LOAD, base_version, delta_versions);
      if (!load_queue_->write(info)) {
        LOG(ERROR) << "Delta load data temp store queue is full, queue size:" << PARTITION_HANDLER_BASE_MAX_QUEUE_SIZE
                   << " partition:" << *partition_ << " load base version:" << base_version;
      }
      spinlock_.unlock();
      return;
    }

    if (base_version_ != base_version) {
      LOG(ERROR) << "Partition:" << *partition_ << " load delta data base version is not current base, base version"
                 << base_version << " current service version:" << base_version_;
      spinlock_.unlock();
      triggerLoadData();
      return;
    }

    status_ = PartitionLoadStatus::DELTA_LOADING;
    spinlock_.unlock();
  }

  // ingest sst data
  for (auto& version : delta_versions) {
    if (std::find(delta_versions_.begin(), delta_versions_.end(), version) != delta_versions_.end()) {
      VLOG(2) << "Load delta data, partition:" << *partition_ << " base version:" << base_version_
              << " delta version:" << version << " has loaded.";
      continue;
    }
    if (ingestDeltaData(version)) {
      delta_versions_.push_back(version);
    }
  }

  {  // end modify status
    folly::SpinLockGuard g(spinlock_);
    status_ = PartitionLoadStatus::DELTA_LOADED;
    database_meta_info_->updateDeltaVersions(partition_, delta_versions_);
  }

  triggerLoadData();
}

void PartitionHandler::getPartitionMetaInfo(PartitionMetaInfo* info) {
  info->setRole(partition_->getRole());
  info->setPartitionId(partition_->getPartitionId());
  info->setDatabaseName(partition_->getDatabaseName());
  info->setTableName(partition_->getTableName());
  info->setHash(partition_->getPartitionHash());
  info->setSize(getPartitionSize());
  info->setReadKps(getPartitionReadKps());
  info->setWriteKps(getPartitionWriteKps());
  info->setReadBytes(getPartitionReadBytes());
  info->setWriteBytes(getPartitionWriteBytes());

  folly::SpinLockGuard g(spinlock_);
  info->setBaseVersion(base_version_);
  info->setDeltaVersions(delta_versions_);

  if (db_) {
    auto replication_db = db_->getReplicationDB().lock();
    if (replication_db) {
      ReplicationDbMetaInfo db_info;
      replication_db->getDbMetaInfo(&db_info);
      info->setDbInfo(db_info);
    }
  }
}

void PartitionHandler::getPropertyKeys(std::vector<std::string>* keys) { ReplicationDB::getPropertyKeys(keys); }

uint64_t PartitionHandler::getProperty(const std::string& key) {
  std::shared_ptr<ReplicationDB> replication_db;
  folly::SpinLockGuard g(spinlock_);
  if (db_) {
    replication_db = db_->getReplicationDB().lock();
    if (replication_db) {
      return replication_db->getProperty(key);
    }
  }
  return 0;
}

void PartitionHandler::forceBaseDataReplication() {
  folly::SpinLockGuard g(spinlock_);
  if (db_) {
    auto replication_db = db_->getReplicationDB().lock();
    if (replication_db) {
      return replication_db->forceBaseDataReplication();
    }
  }
}

bool PartitionHandler::updateRocksdbOptions() {
  folly::SpinLockGuard g(spinlock_);
  bool has_changed = database_manager_->getRocksdbConfigFactory()->hasOptionsChanged(
      partition_->getDatabaseName(), partition_->getTableName(), versioned_options_);
  if (has_changed) {
    auto op_versioned_options = database_manager_->getRocksdbConfigFactory()->getVersionedOptions(
        partition_->getDatabaseName(), partition_->getTableName());
    if (!op_versioned_options.hasValue()) {
      LOG(INFO) << "Can not get options for table: " << partition_->getDatabaseName() << ":"
                << partition_->getTableName();
      return false;
    }
    LOG(INFO) << "Rocksdb options of " << partition_->getDatabaseName() << ":" << partition_->getTableName()
              << " has changed, current version hash: " << versioned_options_.getVersionHash()
              << " new version hash: " << op_versioned_options.value().getVersionHash();
    versioned_options_ = op_versioned_options.value();
    std::string version = database_meta_info_->getVersion(partition_);
    auto delta_versions = database_meta_info_->getDeltaVersions(partition_);
    if (!reloadRocksDbEngine(version, delta_versions)) {
      return false;
    }
  }

  std::weak_ptr<PartitionHandler> handler = shared_from_this();
  timer_evb_->runInEventBaseThread([FLAGS_rocksdb_options_check_interval_ms, handler]() {
    auto partition_handler = handler.lock();
    if (!partition_handler) {
      return;
    }
    partition_handler->timeout_->scheduleTimeout(std::chrono::milliseconds(FLAGS_rocksdb_options_check_interval_ms));
  });
  return true;
}

void PartitionHandler::syncDestoryRocksDbEngine(std::shared_ptr<RocksDbEngine>&& db) {
  auto tmp_db = std::move(db);
  // 保证没有其他调用者引用 db 才能 close db
  while (tmp_db.use_count() > 1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_rocksdb_engine_destory_wait_interval_ms));
    FB_LOG_EVERY_MS(INFO, 1000) << "Partition " << *partition_ << " looping for other callers release db";
  }
  if (FLAGS_finish_rocksdb_processing_operation_time_ms > 0) {
    LOG(INFO) << "Waiting for processing rocksdb operaion finish: " << FLAGS_finish_rocksdb_processing_operation_time_ms
              << " ms";
    std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_finish_rocksdb_processing_operation_time_ms));
  }
  tmp_db->close();

  std::weak_ptr<RocksDbEngine> weak_db = tmp_db;
  tmp_db.reset();
  while (weak_db.lock()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_rocksdb_engine_destory_wait_interval_ms));
    FB_LOG_EVERY_MS(INFO, 1000) << "Partition " << *partition_ << " looping for desotry";
  }
}

bool PartitionHandler::loadRocksDbEngine(const std::string& version, const std::vector<std::string>& delta_versions) {
  std::shared_ptr<RocksDbEngine> db;
  bool ret = createRocksDbEngine(&db, version);
  if (!ret) {
    return false;
  }

  exchangeBaseVersion(db, version, delta_versions);
  return true;
}

bool PartitionHandler::reloadRocksDbEngine(const std::string& version, const std::vector<std::string>& delta_versions) {
  if (db_) {
    syncDestoryRocksDbEngine(std::move(db_));
  }
  return loadRocksDbEngine(version, delta_versions);
}

uint64_t PartitionHandler::getPartitionSize() { return getProperty(PARTITION_SIZE_PROPERTY); }

uint64_t PartitionHandler::getPartitionReadKps() { return getProperty(ROCKSDB_READ_KPS_MIN_1); }

uint64_t PartitionHandler::getPartitionWriteKps() { return getProperty(ROCKSDB_WRITE_KPS_MIN_1); }

uint64_t PartitionHandler::getPartitionReadBytes() { return getProperty(ROCKSDB_READ_BYTES_MIN_1); }

uint64_t PartitionHandler::getPartitionWriteBytes() { return getProperty(ROCKSDB_WRITE_BYTES_MIN_1); }

}  // namespace laser
