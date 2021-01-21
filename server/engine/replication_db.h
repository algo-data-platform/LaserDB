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

#include <fstream>

#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/slice.h"
#include "rocksdb/table.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/utilities/checkpoint.h"
#include "folly/SpinLock.h"
#include "folly/Function.h"

#include "common/laser/laser_entity.h"
#include "common/laser/format.h"
#include "common/laser/if/gen-cpp2/Replicator.h"
#include "common/laser/if/gen-cpp2/laser_types.h"
#include "common/service_router/thrift.h"
#include "common/metrics/metrics.h"

#include "wdt_replicator.h"

namespace laser {

DECLARE_int32(wdt_replicator_abort_timeout_ms);

extern const char ROCKSDB_READ_BYTES_MIN_1[];
extern const char ROCKSDB_WRITE_BYTES_MIN_1[];
extern const char ROCKSDB_READ_KPS_MIN_1[];
extern const char ROCKSDB_WRITE_KPS_MIN_1[];

struct TimeLogExtractor : public rocksdb::WriteBatch::Handler {
 public:
  void LogData(const rocksdb::Slice& blob) override {
    CHECK(blob.size() == sizeof(ms));
    memcpy(&ms, blob.data(), sizeof(ms));
  }
  uint64_t ms;
};

class ExecutorWithTimeout {
 public:
  explicit ExecutorWithTimeout(folly::Executor* executor) : tasks_(), executor_(executor) {}

  template <typename Func, typename Predicate>
  void runIfConditionOrWaitForNotify(Func f, Predicate p, uint64_t timeout_ms) {
    if (p()) {
      executor_->add(std::move(f));
      return;
    }

    auto task = std::make_shared<Task>(std::move(f));
    {
      folly::SpinLockGuard g(spinlock_);
      task->next = std::move(tasks_);
      tasks_ = task;
    }

    if (timeout_ms > 0) {
      std::weak_ptr<Task> weak_task(task);
      folly::futures::sleep(std::chrono::milliseconds(timeout_ms)).via(folly::getGlobalCPUExecutor()).thenValue([
        weak_task = std::move(weak_task),
        executor = executor_
      ] (auto) {
          auto task = weak_task.lock();
          if (task && task->isDone()) {
            executor->add(std::move(task->func));
          }
        });
    }
  }

  void notifyAll() {
    std::shared_ptr<Task> local_tasks;
    {
      folly::SpinLockGuard g(spinlock_);
      local_tasks.swap(tasks_);
    }
    runAllTaskList(std::move(local_tasks));
  }

  ~ExecutorWithTimeout() { runAllTaskList(tasks_); }

 private:
  struct Task {
    template <typename Func>
    explicit Task(Func&& f)
        : func(std::move(f)), next(), has_done(false) {}
    bool isDone() { return !has_done.exchange(true); }

    folly::Function<void()> func;
    std::shared_ptr<Task> next;
    std::atomic<bool> has_done;
  };

  void runAllTaskList(std::shared_ptr<Task> tasks) {
    while (tasks) {
      if (tasks->isDone()) {
        executor_->add(std::move(tasks->func));
      }

      tasks = std::move(tasks->next);
    }
  }

  std::shared_ptr<Task> tasks_;
  folly::Executor* const executor_;
  folly::SpinLock spinlock_;
};

using IteratorCallback = folly::Function<void(rocksdb::Iterator*)>;
using UpdateVersionCallback = folly::Function<void(int64_t db_hash, const std::string& version)>;
class RocksDbBatch {
 public:
  RocksDbBatch() {}
  ~RocksDbBatch() {}
  void iput(const LaserSerializer& key, const LaserValueFormatBase& data);
  void idelete(const LaserSerializer& key);
  inline rocksdb::WriteBatch& getBatch() { return batch_; }

 private:
  rocksdb::WriteBatch batch_;
};

class ReplicationDB : public std::enable_shared_from_this<ReplicationDB> {
 public:
  ReplicationDB(const std::string& data_dir, const rocksdb::Options& options);
  virtual ~ReplicationDB() {
    for (auto &meter : custom_properties_meters_) {
      meter.second->stop();
    }
    close();
    VLOG(3) << "Rocksdb delete, data dir:" << data_dir_;
  }

  void init(folly::EventBase* evb);

  // db opt
  virtual bool open();
  virtual bool close();
  virtual Status read(LaserValueFormatBase* value, const LaserSerializer& key);
  virtual Status write(RocksDbBatch& batch);  // NOLINT
  virtual Status exist(bool* result, std::string* value, const LaserSerializer& key);
  virtual Status delkey(const LaserKeyFormatBase& key);
  virtual Status ingestBaseSst(const std::string& ingest_file);
  virtual Status ingestDeltaSst(const std::string& ingest_file, const std::string& tempdb_path);
  virtual Status dumpSst(const std::string& sst_file_path);
  virtual Status checkpoint(const std::string& checkpoint_path);
  virtual Status compactRange();
  virtual void iterator(IteratorCallback callback);
  virtual inline void setWriteOption(const rocksdb::WriteOptions& options) {
    default_write_options_ = options;
  }
  virtual inline void setReadOption(const rocksdb::ReadOptions& options) {
    default_read_options_ = options;
  }
  virtual uint64_t getProperty(const std::string& key);
  static void getPropertyKeys(std::vector<std::string>* keys);

  // replicator opt
  virtual void startReplicator(uint32_t shard_id, int64_t db_hash, const std::string& service_name,
                               std::shared_ptr<folly::IOExecutor> executor, const DBRole& role,
                               const std::string& version, int64_t node_hash, const std::string& client_address,
                               std::shared_ptr<WdtReplicatorManager> wdt_manager);
  virtual void changeRole(const DBRole& role);
  virtual void setUpdateVersionCallback(UpdateVersionCallback callback) {
    update_version_callback_ = std::move(callback);
  }
  virtual void handleReplicateRequest(
      std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateResponse>>> callback,
      std::unique_ptr<::laser::ReplicateRequest> request);
  virtual void handleReplicateWdtRequest(
      std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateWdtResponse>>> callback,
      std::unique_ptr<::laser::ReplicateWdtRequest> request);
  virtual void getDbMetaInfo(ReplicationDbMetaInfo* info);
  virtual void forceBaseDataReplication();

 private:
  std::string data_dir_;
  rocksdb::Options options_;
  std::unique_ptr<rocksdb::DB> db_{nullptr};

  rocksdb::WriteOptions default_write_options_;
  rocksdb::ReadOptions default_read_options_;

  uint32_t shard_id_;
  int64_t db_hash_;
  std::string replicator_service_name_;
  std::shared_ptr<folly::IOExecutor> executor_;
  apache::thrift::RpcOptions rpc_options_;
  std::shared_ptr<ExecutorWithTimeout> cond_var_;
  std::string version_;
  DBRole role_{DBRole::LEADER};
  int64_t node_hash_;
  std::string client_address_;
  UpdateVersionCallback update_version_callback_;
  std::atomic<int64_t> leader_max_seq_no_{0};
  folly::Synchronized<std::unordered_map<int64_t, std::string>> clients_;

  std::shared_ptr<WdtReplicatorManager> wdt_manager_;
  folly::SpinLock checkpoint_ref_spin_;
  uint32_t checkpoint_ref_count_{0};
  folly::EventBase* evb_;

  std::shared_ptr<metrics::Timers> get_updates_timers_;
  std::shared_ptr<metrics::Timers> write_timers_;
  std::shared_ptr<metrics::Timers> write_timers_without_lock_;
  std::shared_ptr<metrics::Timers> read_timers_;
  std::shared_ptr<metrics::Histograms> pull_rpc_request_latency_;
  std::shared_ptr<metrics::Histograms> pull_rpc_response_latency_;
  std::shared_ptr<metrics::Histograms> whole_replication_latency_;
  std::shared_ptr<metrics::Histograms> apply_updates_numbers_;
  std::shared_ptr<metrics::Timers> apply_updates_timers_;
  std::shared_ptr<metrics::Meter> apply_updates_kps_;
  std::shared_ptr<metrics::Meter> sequence_no_diff_;
  std::shared_ptr<metrics::Histograms> interval_between_write_and_replicate_;
  std::shared_ptr<metrics::Meter> read_bytes_meter_;
  std::shared_ptr<metrics::Meter> write_bytes_meter_;
  std::shared_ptr<metrics::Meter> read_kps_meter_;
  std::shared_ptr<metrics::Meter> write_kps_meter_;
  std::unordered_map<std::string, std::shared_ptr<metrics::Meter>> custom_properties_meters_;

  using TransactionLogIteratorPair = std::pair<std::unique_ptr<rocksdb::TransactionLogIterator>, uint64_t>;
  folly::Synchronized<std::unordered_map<uint64_t, TransactionLogIteratorPair>> cached_iters_;
  std::atomic_bool force_base_data_replication_{false};

 protected:
  virtual void pullFromUpstream();
  virtual void getPullRequest(ReplicateRequest* req, const ReplicateType& type, int64_t max_seq_no = 0);
  virtual void delayPullFromUpstream();
  virtual void applyUpdates(folly::Try<ReplicateResponse>&& try_response);
  virtual bool reachMaxSeqNoDiffLimit(const laser::ReplicateResponse& response);
  virtual void getUpdates(ReplicateResponse* response, std::unique_ptr<::laser::ReplicateRequest> request);
  virtual const service_router::ClientOption getClientOption();
  virtual const service_router::ClientOption getClientOption(int64_t node_hash);
  virtual const Status convertRocksDbStatus(const rocksdb::Status& status);
  virtual Status writeWithSeqNumber(rocksdb::WriteBatch& write_batch, int64_t* seq_no, int64_t write_ms);  // NOLINT
  virtual int64_t getIterHash(int64_t seq_no, int64_t node_hash);
  virtual std::unique_ptr<rocksdb::TransactionLogIterator> getCachedIter(int64_t seq_no, int64_t node_hash);
  virtual void putCachedIter(int64_t seq_no, int64_t node_hash, std::unique_ptr<rocksdb::TransactionLogIterator> iter);
  virtual void cleanIdleCachedIters();
  virtual Status convertIngestFile(const std::string& ingest_file, const std::string& output);
  virtual Status preIngest(std::string* final_ingest_file, const std::string& ingest_file);
  virtual void postIngest(const std::string& ingest_file);
  virtual bool readInt(uint32_t* result, std::ifstream& fin);
  virtual bool readString(std::string* result, uint32_t* length, uint32_t kv_len, std::ifstream& fin);
  virtual Status innerIngestBaseSst(const std::string& ingest_file);
};

}  // namespace laser
