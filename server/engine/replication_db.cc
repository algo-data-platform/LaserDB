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

#include "boost/filesystem.hpp"
#include "folly/ScopeGuard.h"
#include "folly/ExceptionWrapper.h"

#include "common/util.h"
#include "common/laser/status.h"
#include "common/laser/if/gen-cpp2/ReplicatorAsyncClient.h"

#include "replication_db.h"
#include "expire_filter.h"
#include "scoped_key_lock.h"

namespace laser {

DEFINE_int32(replicator_max_server_wait_time_ms, 10 * 1000, "Max wait time before an empty response is returned");

DEFINE_int32(replicator_client_server_timeout_difference_ms, 10 * 1000,
             "The difference between server and client side timeouts");

DEFINE_int32(replicator_max_size_per_response, 1 * 1024 * 1024, "Max size of RocksDB updates a response can contain");

DEFINE_int32(replicator_max_updates_per_response, 500, "Max number of RocksDB updates a response can contain");

DEFINE_int32(replicator_pull_delay_on_error_ms, 5 * 1000,
             "How long to wait before sending the next pull request on error");

DEFINE_int32(replication_max_conn_per_server, 1, "Max number of connection leader server");

DEFINE_bool(replication_reverse_mode_enable, true, "Enable replication reverse mode");

DEFINE_int32(replicator_idle_iter_timeout_ms, 10000, "Iter cache max idle ms");

DEFINE_int32(wdt_replicator_abort_timeout_ms, 60 * 1000, "Wdt replicator abort timeout ms");

DEFINE_int32(delta_batch_load_numbers, 1000, "Delta data batch load numbers");

DEFINE_bool(batch_ingest_format_is_sst, false, "Batch ingest file format is sst or not");

DEFINE_int32(repliction_max_seq_no_diff_time_window_seconds, 5 * 60, "A time window to judge whether the difference" \
            " of sequnce no between leader and follower is big enough to trigger base data replication");

DEFINE_int32(replication_max_seq_no_diff_minute_level, 1, "Max seq no diff minure level");

constexpr char REPLICATION_DB_MODULE_NAME[] = "replication_db";
constexpr char REPLICATION_DB_REPLICATOR_LATENCY[] = "replicator_latency";
constexpr char REPLICATION_DB_REPLICATOR_PULL_RPC_REQUEST_LATENCY[] = "pull_rpc_request_latency";
constexpr char REPLICATION_DB_REPLICATOR_PULL_RPC_RESPONSE_LATENCY[] = "pull_rpc_response_latency";
constexpr char REPLICATION_DB_REPLICATOR_WHOLE_REPLICATION_LATENCY[] = "whole_replication_latency";
constexpr char REPLICATION_DB_REPLICATOR_GET_UPDATES[] = "get_updates";
constexpr char REPLICATION_DB_REPLICATOR_APPLY_UPDATE_NUMBERS[] = "apply_update_number";
constexpr char REPLICATION_DB_REPLICATOR_APPLY_UPDATE_TIMERS[] = "apply_update_timers";
constexpr char REPLICATION_DB_REPLICATOR_APPLY_UPDATE_KPS[] = "apply_update_kps";
constexpr char REPLICATION_DB_REPLICATOR_SEQUENCE_NO_DIFF[] = "sequence_no_diff";
constexpr char REPLICATION_DB_LEADER_INTERVAL_BETWEEN_WRITE_AND_REPLICATE[] = "interval_between_write_and_replicate";
constexpr char REPLICATION_DB_WRITE_TIME[] = "write_timers";
constexpr char REPLICATION_DB_WRITE_TIME_WITHOUT_LOCK[] = "write_timers_without_lock";
constexpr char REPLICATION_DB_READ_TIME[] = "read_timers";
constexpr double REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE = 1.0;
constexpr double REPLICATION_DB_REPLICATOR_TIMER_MIN = 0.0;
constexpr double REPLICATION_DB_REPLICATOR_TIMER_MAX = 1000.0;
constexpr char BATCH_INGEST_CONVERT_FILE_SUBFFIX[] = "_sst";
// The magic number to clean table as the default
constexpr char REPLICATION_DB_REPLICATOR_EMPTY_TABLE_KEY[] = "EMPTY_TABLE_KEY";
constexpr char REPLICATION_DB_REPLICATOR_EMPTY_TABLE_VALUE[] = "EMPTY_TABLE_VALUE";

// Custom properties for table level metrics
constexpr int CUSTOM_PROPERTIES_MINUTE_LEVEL = 1;
constexpr char ROCKSDB_READ_BYTES_MIN_1[] = "rocksdb.read_bytes_min_1";
constexpr char ROCKSDB_WRITE_BYTES_MIN_1[] = "rocksdb.write_bytes_min_1";
constexpr char ROCKSDB_READ_KPS_MIN_1[] = "rocksdb.read_kps_min_1";
constexpr char ROCKSDB_WRITE_KPS_MIN_1[] = "rocksdb.write_kps_min_1";

ReplicationDB::ReplicationDB(const std::string& data_dir, const rocksdb::Options& options) :
    data_dir_(data_dir), options_(options) {
  VLOG(3) << "Rocksdb create, data dir:" << data_dir_;
  get_updates_timers_ = metrics::Metrics::getInstance()->buildTimers(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_GET_UPDATES,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  pull_rpc_request_latency_ = metrics::Metrics::getInstance()->buildHistograms(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_PULL_RPC_REQUEST_LATENCY,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  pull_rpc_response_latency_ = metrics::Metrics::getInstance()->buildHistograms(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_PULL_RPC_RESPONSE_LATENCY,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  whole_replication_latency_ = metrics::Metrics::getInstance()->buildHistograms(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_WHOLE_REPLICATION_LATENCY,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  apply_updates_numbers_ = metrics::Metrics::getInstance()->buildHistograms(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_APPLY_UPDATE_NUMBERS,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  apply_updates_timers_ = metrics::Metrics::getInstance()->buildTimers(
                                             REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_APPLY_UPDATE_TIMERS,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  apply_updates_kps_ = metrics::Metrics::getInstance()->buildMeter(REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_REPLICATOR_APPLY_UPDATE_KPS);
  interval_between_write_and_replicate_ = metrics::Metrics::getInstance()->buildHistograms(
                                             REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_LEADER_INTERVAL_BETWEEN_WRITE_AND_REPLICATE,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  write_timers_ = metrics::Metrics::getInstance()->buildTimers(
                                             REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_WRITE_TIME,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  write_timers_without_lock_ = metrics::Metrics::getInstance()->buildTimers(
                                             REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_WRITE_TIME_WITHOUT_LOCK,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
  read_timers_ = metrics::Metrics::getInstance()->buildTimers(
                                             REPLICATION_DB_MODULE_NAME,
                                             REPLICATION_DB_READ_TIME,
                                             REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
                                             REPLICATION_DB_REPLICATOR_TIMER_MIN,
                                             REPLICATION_DB_REPLICATOR_TIMER_MAX);
}

void ReplicationDB::init(folly::EventBase* evb) {
  evb_ = evb;
  std::vector<int> minute_level{CUSTOM_PROPERTIES_MINUTE_LEVEL};
  read_bytes_meter_ = std::make_shared<metrics::Meter>(REPLICATION_DB_MODULE_NAME,
                                                       ROCKSDB_READ_BYTES_MIN_1, evb_, minute_level);
  read_bytes_meter_->init();
  custom_properties_meters_.insert(std::make_pair(ROCKSDB_READ_BYTES_MIN_1, read_bytes_meter_));

  write_bytes_meter_ = std::make_shared<metrics::Meter>(REPLICATION_DB_MODULE_NAME,
                                                        ROCKSDB_WRITE_BYTES_MIN_1, evb_, minute_level);
  write_bytes_meter_->init();
  custom_properties_meters_.insert(std::make_pair(ROCKSDB_WRITE_BYTES_MIN_1, write_bytes_meter_));

  read_kps_meter_ = std::make_shared<metrics::Meter>(REPLICATION_DB_MODULE_NAME,
                                                     ROCKSDB_READ_KPS_MIN_1, evb_, minute_level);
  read_kps_meter_->init();
  custom_properties_meters_.insert(std::make_pair(ROCKSDB_READ_KPS_MIN_1, read_kps_meter_));

  write_kps_meter_ = std::make_shared<metrics::Meter>(REPLICATION_DB_MODULE_NAME,
                                                      ROCKSDB_WRITE_KPS_MIN_1, evb_, minute_level);
  write_kps_meter_->init();
  custom_properties_meters_.insert(std::make_pair(ROCKSDB_WRITE_KPS_MIN_1, write_kps_meter_));

  sequence_no_diff_ = std::make_shared<metrics::Meter>(REPLICATION_DB_MODULE_NAME,
                                                       REPLICATION_DB_REPLICATOR_SEQUENCE_NO_DIFF, evb_,
                                                       std::vector<int>{
                                                       FLAGS_replication_max_seq_no_diff_minute_level});
  sequence_no_diff_->init();
  custom_properties_meters_.insert(std::make_pair(REPLICATION_DB_REPLICATOR_SEQUENCE_NO_DIFF, sequence_no_diff_));
}

bool ReplicationDB::open() {
  std::string data_dir = common::pathJoin(data_dir_, "data");
  boost::filesystem::path bpath(data_dir);
  if (!boost::filesystem::exists(bpath)) {
    try {
      boost::filesystem::create_directories(bpath);
    }
    catch (...) {
      LOG(ERROR) << "Create rocksdb data dir fail, dir:" << data_dir;
      return false;
    }
  }

  rocksdb::DB* db = nullptr;
  options_.compaction_filter_factory = std::make_shared<ExpireFilterFactory>();
  rocksdb::Status status = rocksdb::DB::Open(options_, data_dir, &db);
  if (!status.ok()) {
    LOG(INFO) << "Create db fail, reason:" << status.ToString();
    return false;
  }
  db_.reset(db);
  clients_.wlock()->clear();
  return true;
}

bool ReplicationDB::close() {
  if (db_) {
    rocksdb::Status status = db_->Close();
    if (!status.ok()) {
      LOG(ERROR) << "Close rocksdb fail, reason: " << status.ToString();
      return false;
    }
  }
  return true;
}

void ReplicationDB::iterator(IteratorCallback callback) {
  rocksdb::ReadOptions read_options;
  read_options.snapshot = db_->GetSnapshot();
  auto iter = db_->NewIterator(read_options);
  SCOPE_EXIT {
    delete iter;
  };
  callback(iter);
  db_->ReleaseSnapshot(read_options.snapshot);
}

Status ReplicationDB::delkey(const LaserKeyFormatBase& key) {
  LaserValueFormatBase value;
  Status status = read(&value, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  if (status == Status::RS_NOT_FOUND) {
    return Status::OK;
  }

  if (!value.decode()) {
    return Status::RS_IO_ERROR;
  }

  RocksDbBatch batch;
  if (value.getType() == ValueType::SET || value.getType() == ValueType::MAP || value.getType() == ValueType::LIST) {
    iterator([this, &key, &batch](auto iter) {
      LaserKeyFormat prefix(key, KeyType::COMPOSITE);
      rocksdb::Slice slice_key(prefix.data(), prefix.length());
      for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
        LaserKeyFormat delete_key(iter->key().data(), iter->key().size());
        batch.idelete(delete_key);
      }
    });
  }

  batch.idelete(key);
  return write(batch);
}

bool ReplicationDB::readInt(uint32_t* result, std::ifstream& fin) {
  char buffer[4];
  memset(buffer, 0, 4);
  fin.read(buffer, 4);
  if (!fin.good() || fin.gcount() != 4) {
    return false;
  }
  memcpy(result, buffer, 4);
  *result = folly::Endian::big(*result);
  return true;
}

bool ReplicationDB::readString(std::string* result, uint32_t* length, uint32_t kv_len, std::ifstream& fin) {
  if (!readInt(length, fin)) {
    return false;
  }

  if (*length > kv_len) {
    return false;
  }
  std::unique_ptr<char[]> data(new char[*length]);
  fin.read(data.get(), *length);
  if (!fin.good() || fin.gcount() != *length) {
    return false;
  }
  result->append(data.get(), *length);
  return true;
}

// 首先将和 mapreduce 输出约定的格式转化为 sst 格式，具体的约定格式为：
// | kv_length | key_length | key | value_length | value |
Status ReplicationDB::convertIngestFile(const std::string& ingest_file, const std::string& output) {
  auto sst_writer = std::make_shared<rocksdb::SstFileWriter>(rocksdb::EnvOptions(), options_);
  rocksdb::Status status = sst_writer->Open(output);
  if (!status.ok()) {
    LOG(INFO) << "Error while opening sst file " << output << ", Error: " << status.ToString();
    return convertRocksDbStatus(status);
  }

  std::ifstream fin(ingest_file, std::ios::binary);
  if (!fin) {
    LOG(ERROR) << "Open local file fail, path:" << ingest_file;
    return Status::RS_ERROR;
  }
  SCOPE_EXIT {
    fin.close();
  };

  fin.seekg(0, fin.end);
  size_t length = fin.tellg();
  fin.seekg(0, fin.beg);

  if (length == 0) {
    std::string key = folly::to<std::string>(REPLICATION_DB_REPLICATOR_EMPTY_TABLE_KEY);
    rocksdb::Slice slice_key(key.data(), key.size());

    LaserValueRawString value(folly::to<std::string>(REPLICATION_DB_REPLICATOR_EMPTY_TABLE_VALUE));
    rocksdb::Slice slice_value(value.data(), value.length());
    sst_writer->Put(slice_key, slice_value);
    status = sst_writer->Finish();
    if (!status.ok()) {
      LOG(ERROR) << "Error while finishing the empty file " << ingest_file << ", Error: " << status.ToString();
      return Status::RS_ERROR;
    }

    return Status::OK;
  }

  size_t offset = 0;
  do {
    uint32_t kv_len = 0;
    if (!readInt(&kv_len, fin)) {
      return Status::RS_ERROR;
    }
    offset += 4;
    if ((kv_len + offset) > length) {
      return Status::RS_ERROR;
    }

    std::string key;
    uint32_t key_len = 0;
    if (!readString(&key, &key_len, kv_len, fin)) {
      return Status::RS_ERROR;
    }
    offset += 4;
    offset += key_len;

    std::string value;
    uint32_t val_len = 0;
    if (!readString(&value, &val_len, kv_len, fin)) {
      return Status::RS_ERROR;
    }
    offset += 4;
    offset += val_len;
    rocksdb::Slice slice_key(key.data(), key.size());
    LaserValueRawString laser_data(value);
    rocksdb::Slice slice_value(laser_data.data(), laser_data.length());
    sst_writer->Put(slice_key, slice_value);
  } while (offset < length);

  status = sst_writer->Finish();
  if (!status.ok()) {
    LOG(ERROR) << "Error while finishing file " << ingest_file << ", Error: " << status.ToString();
    return Status::RS_ERROR;
  }

  return Status::OK;
}

Status ReplicationDB::preIngest(std::string* final_ingest_file, const std::string& ingest_file) {
  *final_ingest_file = ingest_file;
  if (!FLAGS_batch_ingest_format_is_sst) {
    *final_ingest_file = folly::to<std::string>(ingest_file, BATCH_INGEST_CONVERT_FILE_SUBFFIX);
    Status status = convertIngestFile(ingest_file, *final_ingest_file);
    if (Status::OK != status) {
      return status;
    }
  }
  return Status::OK;
}

void ReplicationDB::postIngest(const std::string& ingest_file) {
  std::string final_ingest_file = ingest_file;
  if (!FLAGS_batch_ingest_format_is_sst) {
    final_ingest_file = folly::to<std::string>(ingest_file, BATCH_INGEST_CONVERT_FILE_SUBFFIX);
    boost::filesystem::path origin_file(ingest_file);
    if (boost::filesystem::exists(origin_file)) {
      boost::filesystem::remove_all(origin_file);
      // 删除原有的文件后，创建一个空的文件，防止在重启的时候 hdfs 重新加载文件
      // 此种方式有一定风险就是正在批量加载过程中重启服务可能导致数据导入不完整
      // 通过删除 source_data 下对应的数据表的数据即可完成
      std::ofstream ofs;
      ofs.open(ingest_file, std::ifstream::trunc | std::ifstream::out);
      ofs.close();
    }
  }

  boost::filesystem::path final_sst_file(final_ingest_file);
  if (boost::filesystem::exists(final_ingest_file)) {
    boost::filesystem::remove_all(final_ingest_file);
  }
}

Status ReplicationDB::ingestBaseSst(const std::string& ingest_file) {
  std::string final_ingest_file;
  Status pre_status = preIngest(&final_ingest_file, ingest_file);
  SCOPE_EXIT {
    postIngest(ingest_file);
  };
  if (pre_status != Status::OK) {
    return pre_status;
  }

  return innerIngestBaseSst(final_ingest_file);
}

Status ReplicationDB::innerIngestBaseSst(const std::string& final_ingest_file) {
  rocksdb::IngestExternalFileOptions ifo;
  rocksdb::Status status = db_->IngestExternalFile({final_ingest_file}, ifo);
  if (!status.ok()) {
    LOG(ERROR) << "Error while adding file " << final_ingest_file << " error " << status.ToString();
  }
  return convertRocksDbStatus(status);
}

Status ReplicationDB::ingestDeltaSst(const std::string& ingest_file, const std::string& tempdb_path) {
  boost::filesystem::path bpath(tempdb_path);
  if (boost::filesystem::exists(bpath)) {
    boost::filesystem::remove_all(bpath);
  }
  auto source_db = std::make_shared<ReplicationDB>(tempdb_path, options_);
  if (!source_db->open()) {
    LOG(ERROR) << "Ingest delta sst error, open temp db fail, ingest_file: " << ingest_file;
    return Status::RS_IO_ERROR;
  }

  std::string final_ingest_file;
  Status pre_status = preIngest(&final_ingest_file, ingest_file);
  SCOPE_EXIT {
    postIngest(ingest_file);
  };
  if (pre_status != Status::OK) {
    return pre_status;
  }
  Status status = source_db->innerIngestBaseSst(final_ingest_file);
  if (Status::OK != status) {
    LOG(ERROR) << "Ingest delta sst error while adding file " << final_ingest_file << " error ";
    return status;
  }

  source_db->iterator([this](auto iter) {
    std::unordered_map<std::shared_ptr<LaserKeyFormatBase>, std::shared_ptr<LaserValueFormatBase>> delta_meta_kvs;
    LaserKeyFormatTypePrefix prefix(KeyType::DEFAULT);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    // 对于 string , counter 类型直接将 key -> value 批量 load 到db 即可
    // 对于复合类型，需要将key -> value 保存下来，接下来继续遍历复合类型里的value 值进行替换
    // TODO(zhongxiu): 对于复合类型目前没有分批处理，可能会使用大量内存
    std::shared_ptr<RocksDbBatch> batch;
    uint32_t batch_numbers = 0;
    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      auto base_key = std::make_shared<LaserKeyFormat>(iter->key().data(), iter->key().size());
      auto base_value = std::make_shared<LaserValueFormatBase>(iter->value().data(), iter->value().size());
      if (!base_key->decode()) {
        continue;
      }
      if (!base_value->decode()) {
        continue;
      }

      if (batch_numbers == 0) {
        batch = std::make_shared<RocksDbBatch>();
      }

      if (base_value->getType() == ValueType::RAW_STRING || base_value->getType() == ValueType::COUNTER) {
        batch_numbers++;
        batch->iput(*base_key, *base_value);
        if (batch_numbers >= FLAGS_delta_batch_load_numbers) {
          batch_numbers = 0;
          write(*batch);
        }
      } else {
        delta_meta_kvs[base_key] = base_value;
      }
    }

    if (batch_numbers) {
      write(*batch);
    }

    for (auto& delta : delta_meta_kvs) {
      ScopedKeyLock guard(std::string((*delta.first).data(), (*delta.first).length()));
      if (Status::OK != delkey(*delta.first)) {
        continue;
      }

      RocksDbBatch batch_composite_type;
      LaserKeyFormat prefix(*delta.first, KeyType::COMPOSITE);
      rocksdb::Slice slice_key(prefix.data(), prefix.length());
      for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
        LaserKeyFormatBase base_key(iter->key().data(), iter->key().size());
        LaserValueFormatBase base_value(iter->value().data(), iter->value().size());
        batch_composite_type.iput(base_key, base_value);
      }
      batch_composite_type.iput(*delta.first, *delta.second);
      if (Status::OK != write(batch_composite_type)) {
        continue;
      }
    }
  });

  return Status::OK;
}

Status ReplicationDB::dumpSst(const std::string& sst_file_path) {
  rocksdb::SstFileWriter sst_file_writer(rocksdb::EnvOptions(), options_);
  rocksdb::Status status = sst_file_writer.Open(sst_file_path);
  if (!status.ok()) {
    LOG(INFO) << "Error while opening file " << sst_file_path << ", Error: " << status.ToString();
    return convertRocksDbStatus(status);
  }

  iterator([this, &sst_file_writer](auto iter) {
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
      if (!iter->status().ok()) {
        continue;
      }
      sst_file_writer.Add(iter->key(), iter->value());
    }
  });

  status = sst_file_writer.Finish();
  if (!status.ok()) {
    LOG(ERROR) << "Error while finishing file " << sst_file_path << ", Error: " << status.ToString();
    return convertRocksDbStatus(status);
  }

  return convertRocksDbStatus(status);
}

void RocksDbBatch::iput(const LaserSerializer& key, const LaserValueFormatBase& data) {
  rocksdb::Slice slice_key(key.data(), key.length());
  rocksdb::Slice slice_value(data.data(), data.length());
  batch_.Put(slice_key, slice_value);
}

void RocksDbBatch::idelete(const LaserSerializer& key) {
  rocksdb::Slice slice_key(key.data(), key.length());
  batch_.Delete(slice_key);
}

Status ReplicationDB::checkpoint(const std::string& checkpoint_path) {
  rocksdb::Checkpoint* checkpoint;
  rocksdb::Status status = rocksdb::Checkpoint::Create(db_.get(), &checkpoint);

  SCOPE_EXIT {
    delete checkpoint;
  };

  if (!status.ok()) {
    LOG(INFO) << "Error while create checkpoint, checkpoint path:" << checkpoint_path
              << ", Error: " << status.ToString();
    return convertRocksDbStatus(status);
  }

  status = checkpoint->CreateCheckpoint(checkpoint_path);
  if (!status.ok()) {
    LOG(INFO) << "Error while create checkpoint, checkpoint path:" << checkpoint_path
              << ", Error: " << status.ToString();
    return convertRocksDbStatus(status);
  }

  return convertRocksDbStatus(status);
}

Status ReplicationDB::write(RocksDbBatch& batch) {
  if (role_ == DBRole::FOLLOWER) {
    return Status::RS_WRITE_IN_FOLLOWER;
  }
  metrics::Timer write_timer(write_timers_.get());
  rocksdb::WriteBatch& write_batch = batch.getBatch();
  if (write_bytes_meter_) {
    auto data_size = write_batch.GetDataSize();
    write_bytes_meter_->mark(static_cast<double>(data_size));
  }

  if (write_kps_meter_) {
    write_kps_meter_->mark(static_cast<double>(write_batch.Count()));
  }
  int64_t seq_no = 0;
  return writeWithSeqNumber(write_batch, &seq_no, 0);
}

Status ReplicationDB::writeWithSeqNumber(rocksdb::WriteBatch& write_batch, int64_t* seq_no, int64_t write_ms) {
  metrics::Timer write_without_lock_timer(write_timers_without_lock_.get());
  if (!write_ms) {
    write_ms = static_cast<int64_t>(common::currentTimeInMs());
  }
  write_batch.PutLogData(rocksdb::Slice(reinterpret_cast<const char*>(&write_ms), sizeof(write_ms)));
  rocksdb::Status status = db_->Write(default_write_options_, &write_batch);
  if (!status.ok()) {
    VLOG(10) << "Batch write value:" << status.ToString();
  } else {
    if (cond_var_) {
      cond_var_->notifyAll();
    }
  }
  if (seq_no != nullptr) {
    *seq_no = db_->GetLatestSequenceNumber();
  }
  return convertRocksDbStatus(status);
}

Status ReplicationDB::read(LaserValueFormatBase* value, const LaserSerializer& key) {
  metrics::Timer read_time(read_timers_.get());
  rocksdb::Slice slice_key(key.data(), key.length());
  rocksdb::Status status = db_->Get(default_read_options_, slice_key, value->getRawBuffer());
  if (!status.ok()) {
    VLOG(10) << "Get value:" << status.ToString();
  }

  if (read_bytes_meter_) {
    read_bytes_meter_->mark(static_cast<double>(value->length()));
  }

  if (read_kps_meter_) {
    read_kps_meter_->mark();
  }
  return convertRocksDbStatus(status);
}

Status ReplicationDB::exist(bool* result, std::string* value, const LaserSerializer& key) {
  rocksdb::Slice slice_key(key.data(), key.length());
  bool ret;
  bool value_found;
  ret = db_->KeyMayExist(default_read_options_, slice_key, value, &value_found);
  // ret is ture and value is really find then the result is true
  *result = ret & value_found;
  return Status::OK;
}

void ReplicationDB::startReplicator(uint32_t shard_id, int64_t db_hash, const std::string& service_name,
                                    std::shared_ptr<folly::IOExecutor> executor, const DBRole& role,
                                    const std::string& version, int64_t node_hash,
                                    const std::string& client_address,
                                    std::shared_ptr<WdtReplicatorManager> wdt_manager) {
  shard_id_ = shard_id;
  db_hash_ = db_hash;
  replicator_service_name_ = service_name;
  executor_ = executor;
  cond_var_ = std::make_shared<ExecutorWithTimeout>(executor_.get());
  version_ = version;
  node_hash_ = node_hash;
  client_address_ = client_address;
  wdt_manager_ = wdt_manager;
  rpc_options_.setTimeout(std::chrono::milliseconds(FLAGS_replicator_max_server_wait_time_ms +
                                                    FLAGS_replicator_client_server_timeout_difference_ms));
  role_ = role;
  if (role == DBRole::FOLLOWER) {
    pullFromUpstream();
  }

  cleanIdleCachedIters();
}

void ReplicationDB::changeRole(const DBRole& role) {
  if (role_ == role) {
    VLOG(5) << "Db " << db_hash_ << " role has is " << role;
    return;
  }
  LOG(INFO) << "Db " << db_hash_ << " role changed " << role;
  role_ = role;
  if (role_ == DBRole::FOLLOWER) {
    cached_iters_.wlock()->clear();
    pullFromUpstream();
  } else {
    clients_.wlock()->clear();
  }
}

bool ReplicationDB::reachMaxSeqNoDiffLimit(const laser::ReplicateResponse& response) {
  int64_t latest_seq_no = db_->GetLatestSequenceNumber();
  uint64_t seq_no_diff = std::abs(response.get_max_seq_no() - latest_seq_no);
  VLOG_EVERY_N(5, 2000) << "Db " << db_hash_ << " version " << version_ << " seq_no_diff: " << seq_no_diff
                        << " latest_seq_no: " << latest_seq_no;
  uint64_t seq_no_diff_min = 0;
  if (sequence_no_diff_) {
    sequence_no_diff_->mark(static_cast<double>(seq_no_diff));
    seq_no_diff_min = static_cast<uint64_t>(sequence_no_diff_->getMinuteRate(
                                              FLAGS_replication_max_seq_no_diff_minute_level));
  }

  uint64_t update_kps_min = static_cast<uint64_t>(apply_updates_kps_->getMinuteRate(
                                                   FLAGS_replication_max_seq_no_diff_minute_level));
  if (update_kps_min != 0 && seq_no_diff_min != 0) {
    if (seq_no_diff_min > update_kps_min * FLAGS_repliction_max_seq_no_diff_time_window_seconds) {
      LOG(INFO) << "Db " << db_hash_ << " version " << version_ << " has reach max seq no diff limit. "
                << "seq_no_diff_min_" << FLAGS_replication_max_seq_no_diff_minute_level << ": " << seq_no_diff_min
                << " update_kps_min_" << FLAGS_replication_max_seq_no_diff_minute_level << ": " << update_kps_min;
      return true;
    }
  }

  return false;
}

void ReplicationDB::applyUpdates(folly::Try<ReplicateResponse>&& try_response) {
  metrics::Timer apply_update_timer(apply_updates_timers_.get());
  if (force_base_data_replication_) {
    if (update_version_callback_) {
      update_version_callback_(db_hash_, version_);
    }
    LOG(INFO) << "Db " << db_hash_ << " version " << version_ << " trigger base data replication manually";
    return;
  }

  bool delay_next_pull = false;
  if (try_response.hasException()) {
    delay_next_pull = true;
    try {
      try_response.exception().throw_exception();
    }
    catch (const LaserException& ex) {
      LOG(ERROR) << "Db " << db_hash_ << " laserException: " << ex.get_message();
      if (ex.get_status() == Status::RP_SOURCE_WAL_LOG_REMOVED) {
        if (update_version_callback_) {
          update_version_callback_(db_hash_, version_);
        }
        LOG(INFO) << "Db " << db_hash_ << " receive laserException: " << ex.get_message()
                  << " trigger base data replication";
        return;
      }
    }
    catch (const std::exception& ex) {
      LOG(ERROR) << "Db " << db_hash_ << " std::exception: " << ex.what();
    }
  } else {
    auto& response = try_response.value();
    auto timestamp_ptr = response.get_timestamp();
    if (timestamp_ptr) {
      int64_t response_latency = static_cast<int64_t>(common::currentTimeInMs()) - *timestamp_ptr;
      if (response_latency < 0) {
        response_latency = 0;
      }
      pull_rpc_response_latency_->addValue(static_cast<double>(response_latency));
    }

    if (response.version != version_) {  // base version update
      if (update_version_callback_) {
        update_version_callback_(db_hash_, response.version);
      }
      LOG(INFO) << "Database db_hash:" << db_hash_ << " base version has update version :" << version_ << " to "
                << response.version;
      return;
    }

    leader_max_seq_no_ = response.max_seq_no;
    auto latency_metric = metrics::Metrics::getInstance()->buildHistograms(
        REPLICATION_DB_MODULE_NAME, REPLICATION_DB_REPLICATOR_LATENCY, REPLICATION_DB_REPLICATOR_TIMER_BUCKET_SIZE,
        REPLICATION_DB_REPLICATOR_TIMER_MIN, REPLICATION_DB_REPLICATOR_TIMER_MAX);
    int64_t current_seq_no = 0;
    apply_updates_numbers_->addValue(static_cast<double>(response.updates.size()));
    for (auto& update : response.updates) {
      auto byteRange = update.raw_data.coalesce();
      rocksdb::WriteBatch write_batch(std::string(reinterpret_cast<const char*>(byteRange.data()), byteRange.size()));
      int64_t latency = static_cast<uint64_t>(common::currentTimeInMs()) - update.timestamp;
      if (latency < 0) {
        latency = 0;
      }
      apply_updates_kps_->mark(static_cast<double>(write_batch.Count()));
      latency_metric->addValue(static_cast<double>(latency));
      auto status = writeWithSeqNumber(write_batch, &current_seq_no, update.timestamp);
      if (status != Status::OK) {
        LOG(ERROR) << "Failed to apply updates to SLAVE " << db_hash_ << " " << status;
        delay_next_pull = true;
        break;
      }
    }
    VLOG(5) << "Db " <<  db_hash_ << " success apply updates, next pull seqno:" <<  db_->GetLatestSequenceNumber();

    if (reachMaxSeqNoDiffLimit(response)) {
      if (update_version_callback_) {
        update_version_callback_(db_hash_, response.version);
      }
      LOG(INFO) << "Db " << db_hash_ << " version " << version_
                << " reach max seq diff limit, trigger base data replication";
      return;
    }
  }

  if (delay_next_pull) {
    delayPullFromUpstream();
  } else {
    pullFromUpstream();
  }
}

void ReplicationDB::pullFromUpstream() {
  if (role_ == DBRole::LEADER) {
    return;
  }

  if (!db_) {
    delayPullFromUpstream();
    return;
  }

  ReplicateRequest req;
  getPullRequest(&req, ReplicateType::FORWARD, 0);
  auto rpc_options = rpc_options_;
  std::weak_ptr<ReplicationDB> weak_db = shared_from_this();
  VLOG(5) << "Start pull updates from " << db_hash_ << " seq_number:" << req.seq_no;

  int64_t request_timestamp = req.timestamp;
  auto process_request = [weak_db, request_timestamp](folly::Try<ReplicateResponse>&& t) {
    auto db = weak_db.lock();
    if (db == nullptr) {
      return;
    }
    db->applyUpdates(std::move(t));
    int64_t whole_latency = static_cast<int64_t>(common::currentTimeInMs()) - request_timestamp;
    if (whole_latency < 0) {
      whole_latency = 0;
    }
    db->whole_replication_latency_->addValue(static_cast<double>(whole_latency));
  };

  auto send_request = [&rpc_options, &req, this, weak_db, process_request = std::move(process_request) ](auto client) {
    client->future_replicate(rpc_options, req).via(executor_.get()).then(std::move(process_request));
  };

  bool ret = service_router::thriftServiceCall<ReplicatorAsyncClient>(getClientOption(), std::move(send_request));
  if (!ret) {
    delayPullFromUpstream();
  }
}

void ReplicationDB::getPullRequest(ReplicateRequest* req, const ReplicateType& type, int64_t max_seq_no) {
  req->seq_no = db_->GetLatestSequenceNumber();
  req->db_hash = db_hash_;
  req->max_wait_ms = FLAGS_replicator_max_server_wait_time_ms;
  req->max_size = static_cast<int64_t>(FLAGS_replicator_max_size_per_response);
  req->type = type;
  req->client_address = client_address_;
  req->node_hash = node_hash_;
  req->max_seq_no = max_seq_no;
  req->version = version_;
  req->set_timestamp(static_cast<int64_t>(common::currentTimeInMs()));
}

void ReplicationDB::delayPullFromUpstream() {
  auto eb = executor_->getEventBase();
  std::weak_ptr<ReplicationDB> weak_db = shared_from_this();
  eb->runInEventBaseThread([
    eb,
    weak_db = std::move(weak_db)
  ]() {
    eb->runAfterDelay([
      weak_db = std::move(weak_db)
    ] {
      auto db = weak_db.lock();
      if (db == nullptr) {
        return;
      }
      db->pullFromUpstream();
    }, FLAGS_replicator_pull_delay_on_error_ms);
  });
}

const service_router::ClientOption ReplicationDB::getClientOption() {
  service_router::ClientOption option;
  option.setServiceName(replicator_service_name_);
  option.setProtocol(service_router::ServerProtocol::THRIFT);
  option.setMaxConnPerServer(FLAGS_replication_max_conn_per_server);
  option.setShardId(shard_id_);
  return option;
}

const service_router::ClientOption ReplicationDB::getClientOption(int64_t node_hash) {
  service_router::ServerAddress target_address;
  clients_.withRLock([node_hash, &target_address](auto& clients) {
    if (clients.find(node_hash) != clients.end()) {
      std::string host;
      uint16_t port = 0;
      folly::split(':', clients.at(node_hash), host, port);
      target_address.setHost(host);
      target_address.setPort(port);
    }
  });
  service_router::ClientOption option;
  option.setServiceName(replicator_service_name_);
  option.setProtocol(service_router::ServerProtocol::THRIFT);
  option.setMaxConnPerServer(FLAGS_replication_max_conn_per_server);
  option.setTargetServerAddress(target_address);
  return option;
}

void ReplicationDB::getUpdates(ReplicateResponse* response, std::unique_ptr<::laser::ReplicateRequest> request) {
  response->version = version_;
  if (request->version != version_) {
    LOG(INFO) << folly::to<std::string>("Db ", request->db_hash, " version has update to ", version_);
    return;
  }
  if (role_ == DBRole::FOLLOWER) {
    throwLaserException(Status::RP_ROLE_ERROR, folly::to<std::string>("Db ", request->db_hash, " role is follower"));
  }

  std::string& client_address = request->client_address;
  clients_.withWLock([&client_address, node_hash = request->node_hash ](auto & clients) {
                                                                         clients[node_hash] = client_address;
                                                                       });
  const auto expected_seq_no = request->seq_no + 1;
  std::unique_ptr<rocksdb::TransactionLogIterator> iter;
  auto cache_iter = getCachedIter(expected_seq_no, request->node_hash);
  rocksdb::Status status;
  if (cache_iter == nullptr || !cache_iter->Valid()) {
    status = db_->GetUpdatesSince(expected_seq_no, &iter);
    VLOG(5) << "Pull updates from " << db_hash_ << " with error: " << status.ToString()
            << " seq_number:" << expected_seq_no;
    if (!status.ok() && !status.IsNotFound()) {
      throwLaserException(Status::RP_SOURCE_READ_ERROR,
                          folly::to<std::string>("Pull updates from", db_hash_, " with error: ", status.ToString()));
    }
  } else {
    iter = std::move(cache_iter);
    VLOG(5) << "Pull updates from " << db_hash_ << " use cache iter, "
            << " seq_number:" << expected_seq_no;
  }

  uint32_t batch_numbers = 0;
  uint64_t update_size = 0;
  for (int32_t i = 0; update_size < request->max_size && iter && iter->Valid(); ++i, iter->Next()) {
    auto result = iter->GetBatch();
    Update update;
    const auto& str = result.writeBatchPtr->Data();
    update.raw_data = std::move(*folly::IOBuf::copyBuffer(str.data(), str.size()));
    TimeLogExtractor extractor;
    auto ret = result.writeBatchPtr->Iterate(&extractor);
    if (ret.ok()) {
      update.timestamp = extractor.ms;
    } else {
      update.timestamp = 0;
    }
    response->updates.emplace_back(std::move(update));
    batch_numbers += result.writeBatchPtr->Count();
    update_size += result.writeBatchPtr->GetDataSize();

    int64_t latency = static_cast<int64_t>(common::currentTimeInMs()) - extractor.ms;
    if (latency < 0) {
      latency = 0;
    }
    interval_between_write_and_replicate_->addValue(static_cast<double>(latency));
  }

  // not found 状态证明已经同步完成，属于正常状态
  if (static_cast<uint32_t>(response->updates.size()) == 0 && !status.IsNotFound()) {
    throwLaserException(Status::RP_SOURCE_WAL_LOG_REMOVED,
                        folly::to<std::string>("Pull updates from ", db_hash_, " wal log has removed"));
  }
  response->max_seq_no = db_->GetLatestSequenceNumber();
  VLOG(5) << "Db " << db_hash_ << " get updates:" << response->updates.size();
  putCachedIter(expected_seq_no + batch_numbers, request->node_hash, std::move(iter));
}

void ReplicationDB::handleReplicateRequest(
    std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateResponse>>> callback,
    std::unique_ptr<::laser::ReplicateRequest> request) {
  auto db = shared_from_this();
  std::weak_ptr<ReplicationDB> weak_db = db;
  auto seq_no = static_cast<rocksdb::SequenceNumber>(request->seq_no);
  auto timeout = request->max_wait_ms;
  VLOG(5) << "handle replication request, hash:" << request->db_hash
          << " from:" << request->client_address << " seq_no:" << request->seq_no;

  auto response_callback =
      [ request = std::move(request), callback = std::move(callback), weak_db = std::move(weak_db) ]() mutable {
    auto db = weak_db.lock();
    if (db == nullptr) {
      LaserException ex = createLaserException(Status::RP_SOURCE_DB_REMOVED,
                                               folly::to<std::string>("Db ", request->db_hash, " has been removed"));
      callback->exception(std::move(ex));
      return;
    }

    int64_t request_timestamp = request->get_timestamp();
    int64_t replication_rpc_request_latency = static_cast<int64_t>(common::currentTimeInMs()) - request_timestamp;
    if (replication_rpc_request_latency < 0) {
      replication_rpc_request_latency = 0;
    }
    db->pull_rpc_request_latency_->addValue(static_cast<double>(replication_rpc_request_latency));

    metrics::Timer metric_time(db->get_updates_timers_.get());
    ReplicateResponse response;
    try {
      db->getUpdates(&response, std::move(request));
      response.set_timestamp(static_cast<int64_t>(common::currentTimeInMs()));
      callback->result(std::move(response));
    }
    catch (const LaserException& ex) {
      callback->exception(std::move(ex));
    }
  };

  auto predicate = [ seq_no, db = std::move(db) ]() {
    return db->db_->GetLatestSequenceNumber() > seq_no;
  };
  cond_var_->runIfConditionOrWaitForNotify(std::move(response_callback), std::move(predicate), timeout);
}

void ReplicationDB::handleReplicateWdtRequest(
    std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateWdtResponse>>> callback,
    std::unique_ptr<::laser::ReplicateWdtRequest> request) {
  if (!db_) {
    LaserException ex = createLaserException(Status::RP_SOURCE_DB_REMOVED,
                                             folly::to<std::string>("Db ", request->db_hash, " has been removed"));
    callback->exception(std::move(ex));
    return;
  }

  // checkpoint 对于多个 follower 的情况采用引用计数的方式共享一个 checkpoint version
  std::string checkpoint_path = common::pathJoin(data_dir_, "checkpoint");
  boost::filesystem::path bpath(checkpoint_path);
  {
    folly::SpinLockGuard g(checkpoint_ref_spin_);
    if (!boost::filesystem::exists(bpath)) {
      Status status = checkpoint(common::pathJoin(data_dir_, "checkpoint"));
      if (status != Status::OK) {
        LaserException ex = createLaserException(status,
            folly::to<std::string>("Db checkpoint fail, db_hash:", request->db_hash));
        callback->exception(std::move(ex));
        return;
      }
    }
    checkpoint_ref_count_++;
  }

  LOG(INFO) << "Create sender for db:" << request->db_hash
            << " version:" << request->version << " wdt_url:" << request->wdt_url;
  auto wdt = std::make_shared<laser::WdtReplicator>(wdt_manager_, folly::to<std::string>(request->db_hash),
                                                    folly::to<std::string>(request->db_hash),
                                                    FLAGS_wdt_replicator_abort_timeout_ms);
  std::weak_ptr<ReplicationDB> weak_db = shared_from_this();

  auto sender_callback = [weak_db, bpath, callback = std::move(callback)](std::string& name_space,
       std::string& ident,  facebook::wdt::ErrorCode error) mutable {
    auto db = weak_db.lock();
    if (!db) {
      LaserException ex = createLaserException(Status::RP_SOURCE_DB_REMOVED,
                                               folly::to<std::string>("Db has been removed"));
      callback->exception(std::move(ex));
      return;
    }

    folly::SpinLockGuard g(db->checkpoint_ref_spin_);
    db->checkpoint_ref_count_--;
    if (db->checkpoint_ref_count_ == 0 && boost::filesystem::exists(bpath)) {
      try {
        boost::filesystem::remove_all(bpath);
        VLOG(5) << "Remove db checkpoint, db_hash:" << db->db_hash_;
      } catch(...) {
        LOG(ERROR) << "Remove db checkpoint fail, db_hash:" << db->db_hash_
                   << " path:" << bpath.string();
      }
    }

    if (error != facebook::wdt::ErrorCode::OK) {
      LaserException ex = createLaserException(Status::RP_SOURCE_READ_ERROR,
          folly::to<std::string>("Db ", db->db_hash_, " send file fail, error", facebook::wdt::errorCodeToStr(error)));
      callback->exception(std::move(ex));
    } else {
      ReplicateWdtResponse response;
      response.send_success = true;
      callback->result(std::move(response));
    }
  };
  wdt->sender(request->wdt_url, checkpoint_path, std::move(sender_callback));
}

void ReplicationDB::getDbMetaInfo(ReplicationDbMetaInfo* info) {
  if (!db_) {
    return;
  }
  uint64_t seq_no = db_->GetLatestSequenceNumber();
  info->setSeqNo(seq_no);
  if (role_ == DBRole::FOLLOWER) {
    info->setReplicateLag(leader_max_seq_no_ - seq_no);
  }
}

void ReplicationDB::forceBaseDataReplication() {
  bool expected = false;
  if (force_base_data_replication_.compare_exchange_strong(expected, true)) {
    LOG(INFO) << "Db " << db_hash_ << " version " << version_ << " force_base_data_replication flag is set manually";
  }
}

int64_t ReplicationDB::getIterHash(int64_t seq_no, int64_t node_hash) {
  std::string hash_str = folly::to<std::string>(seq_no, node_hash);
  return CityHash64WithSeed(hash_str.data(), hash_str.size(), 0);
}

std::unique_ptr<rocksdb::TransactionLogIterator> ReplicationDB::getCachedIter(int64_t seq_no, int64_t node_hash) {
  int64_t hash_key = getIterHash(seq_no, node_hash);
  return cached_iters_.withWLock([hash_key, node_hash, seq_no](auto& iters)
      -> std::unique_ptr<rocksdb::TransactionLogIterator> {
    auto iter = iters.find(hash_key);
    if (iter == iters.end()) {
      VLOG(5) << "Find cache iter fail, seq_no:" << seq_no
              << " node hash:" << node_hash;
      return nullptr;
    }

    auto ret = std::move(iter->second.first);
    iters.erase(hash_key);
    if (ret && !ret->Valid()) {
      ret->Next();
      if (!ret->Valid()) {
        ret.reset(nullptr);
        VLOG(5) << "Find cache iter, seq_no:" << seq_no
                << " node hash:" << node_hash << " is invalid";
      }
    }
    return ret;
  });
}

void ReplicationDB::putCachedIter(int64_t seq_no, int64_t node_hash,
                                  std::unique_ptr<rocksdb::TransactionLogIterator> iter) {
  int64_t hash_key = getIterHash(seq_no, node_hash);
  cached_iters_.withWLock([hash_key, &iter](auto& iters) mutable {
    iters.emplace(hash_key, std::make_pair(std::move(iter), static_cast<uint64_t>(common::currentTimeInMs())));
  });
}

void ReplicationDB::cleanIdleCachedIters() {
  uint64_t now = static_cast<uint64_t>(common::currentTimeInMs());
  cached_iters_.withWLock([now](auto& iters) {
    auto itor = iters.begin();
    while (itor != iters.end()) {
      if (itor->second.second + FLAGS_replicator_idle_iter_timeout_ms < now) {
        itor = iters.erase(itor);
        continue;
      }
      ++itor;
    }
  });
  auto eb = executor_->getEventBase();
  std::weak_ptr<ReplicationDB> weak_db = shared_from_this();
  eb->runInEventBaseThread([
    eb,
    weak_db = std::move(weak_db)
  ]() {
    eb->runAfterDelay([
      weak_db = std::move(weak_db)
    ] {
      auto db = weak_db.lock();
      if (db == nullptr) {
        return;
      }
      db->cleanIdleCachedIters();
    }, FLAGS_replicator_idle_iter_timeout_ms);
  });
}

uint64_t ReplicationDB::getProperty(const std::string& key) {
  uint64_t value = 0;
  auto found = custom_properties_meters_.find(key);
  if (found != custom_properties_meters_.end()) {
    value = static_cast<uint64_t>(found->second->getMinuteRate(CUSTOM_PROPERTIES_MINUTE_LEVEL));
  } else {
    rocksdb::Slice slice_key(key.data(), key.length());
    db_->GetIntProperty(slice_key, &value);
  }
  return value;
}

void ReplicationDB::getPropertyKeys(std::vector<std::string>* keys) {
  std::vector<std::string> rocksdb_keys({
    "rocksdb.num-immutable-mem-table",
    "rocksdb.num-immutable-mem-table-flushed",
    "rocksdb.mem-table-flush-pending",
    "rocksdb.compaction-pending",
    "rocksdb.background-errors",
    "rocksdb.cur-size-active-mem-table",
    "rocksdb.cur-size-all-mem-tables",
    "rocksdb.size-all-mem-tables",
    "rocksdb.num-entries-active-mem-table",
    "rocksdb.num-entries-imm-mem-tables",
    "rocksdb.num-deletes-active-mem-table",
    "rocksdb.num-deletes-imm-mem-tables",
    "rocksdb.estimate-num-keys",
    "rocksdb.estimate-table-readers-mem",
    "rocksdb.is-file-deletions-enabled",
    "rocksdb.num-snapshots",
    "rocksdb.oldest-snapshot-time",
    "rocksdb.num-live-versions",
    "rocksdb.current-super-version-number",
    "rocksdb.estimate-live-data-size",
    "rocksdb.min-log-number-to-keep",
    "rocksdb.min-obsolete-sst-number-to-keep",
    // WARNING: may slow down online queries if there are too many files.
    // "rocksdb.total-sst-files-size",
    "rocksdb.live-sst-files-size",
    "rocksdb.base-level",
    "rocksdb.estimate-pending-compaction-bytes",
    "rocksdb.num-running-compactions",
    "rocksdb.num-running-flushes",
    "rocksdb.actual-delayed-write-rate",
    "rocksdb.is-write-stopped",
    "rocksdb.estimate-oldest-key-time",
    "rocksdb.block-cache-capacity",
    "rocksdb.block-cache-usage",
    "rocksdb.block-cache-pinned-usage",
    // custom properties
    ROCKSDB_READ_BYTES_MIN_1,
    ROCKSDB_WRITE_BYTES_MIN_1,
    ROCKSDB_READ_KPS_MIN_1,
    ROCKSDB_WRITE_KPS_MIN_1,
    REPLICATION_DB_REPLICATOR_SEQUENCE_NO_DIFF,
  });

  *keys = rocksdb_keys;
}

Status ReplicationDB::compactRange() {
  rocksdb::CompactRangeOptions coptions;
  return convertRocksDbStatus(db_->CompactRange(coptions, nullptr, nullptr));
}

const Status ReplicationDB::convertRocksDbStatus(const rocksdb::Status& status) {
  if (status.ok()) {
    return Status::OK;
  } else if (status.IsNotFound()) {
    return Status::RS_NOT_FOUND;
  } else if (status.IsCorruption()) {
    return Status::RS_CORRUPTION;
  } else if (status.IsNotSupported()) {
    return Status::RS_NOT_SUPPORTED;
  } else if (status.IsInvalidArgument()) {
    return Status::RS_INVALID_ARGUMENT;
  } else if (status.IsIOError()) {
    return Status::RS_IO_ERROR;
  } else if (status.IsMergeInProgress()) {
    return Status::RS_MERGE_INPROGRESS;
  } else if (status.IsIncomplete()) {
    return Status::RS_IN_COMPLETE;
  } else if (status.IsShutdownInProgress()) {
    return Status::RS_SHUTDOWN_INPROGRESS;
  } else if (status.IsTimedOut()) {
    return Status::RS_TIMEDOUT;
  } else if (status.IsAborted()) {
    return Status::RS_ABORTED;
  } else if (status.IsBusy()) {
    return Status::RS_BUSY;
  } else if (status.IsExpired()) {
    return Status::RS_EXPIRED;
  } else if (status.IsTryAgain()) {
    return Status::RS_TRYAGAIN;
  } else if (status.IsCompactionTooLarge()) {
    return Status::RS_COMPACTION_TOO_LARGE;
  } else {
    return Status::RS_ERROR;
  }
}

}  // namespace laser
