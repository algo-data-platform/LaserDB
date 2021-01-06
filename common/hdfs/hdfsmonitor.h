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
 * @author liubang <it.liubang@gmail.com>
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#pragma once

#include <string>
#include <vector>
#include "common/util.h"
#include "folly/Synchronized.h"
#include "folly/io/async/AsyncTimeout.h"
#include "folly/io/async/ScopedEventBaseThread.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "folly/SpinLock.h"
#include "hdfs.h"
#include "http_client.h"

namespace hdfs {

using NotifyCallback = folly::Function<void(const std::vector<std::string>&)>;

enum class HdfsFileSyncStatus {
  SYNCED,
  SYNCHING,
  SYNC_FAILED
};
enum class HdfsMetadataSyncStatus {
  SYNCED,
  SYNCHING,
  SYNC_FAILED
};

class HdfsMonitorManager;
class HdfsMonitor : public std::enable_shared_from_this<HdfsMonitor> {
 public:
  HdfsMonitor(const std::string& dir_path, const std::string& dest_path, uint32_t timeval, NotifyCallback cb,
              std::weak_ptr<HdfsMonitorManager> weak_manager);
  virtual ~HdfsMonitor();
  virtual void start();
  // 实例化 monitor 必须调用 destroy 异步回收资源
  virtual void destroy();
  virtual void addFilter(const std::vector<std::string>& path_prefixes);
  virtual void setFilter(const std::vector<std::string>& path_prefixes);
  virtual void delFilter(const std::vector<std::string>& path_prefixes);

 private:
  std::string dir_path_;  // 监控的目录
  std::string dest_path_;
  int timeval_;  // 监控间隔
  NotifyCallback cb_;
  std::weak_ptr<HdfsMonitorManager> weak_manager_;

  HdfsCmd hdfscmd_;
  std::unique_ptr<folly::AsyncTimeout> timeout_;
  folly::SpinLock spinlock_;
  std::atomic<bool> is_start_{false};

  HdfsMetadataSyncStatus metadta_sync_status_{HdfsMetadataSyncStatus::SYNCED};
  std::string metadata_info_;
  std::unordered_map<std::string, HdfsFileSyncStatus> sync_files_;
  std::unordered_map<std::string, std::string> file_checksum_;
  std::vector<std::string> sync_filter_prefixes_;
  std::shared_ptr<metrics::Counter> checksum_info_error_counter_;
  std::shared_ptr<metrics::Counter> download_hdfs_file_fail_counter_;
  void getDatabaseNameTableName(const std::string& dir_path,
                                std::string* database_name, std::string* table_name);
  void watchAndDoMonitor();
  void syncMetaData();
  void refreshMetadataInfo(std::string metadata_path, ResultStatus *status);
  void syncMonitorFiles();
  bool getMonitorFiles(std::unordered_map<std::string, std::string>* monitor_files);
  void updateSyncStatus(const std::string& file, const HdfsFileSyncStatus& status);
  bool isSynching();
  void downloadFile(const std::string& file);
};

// eventbase 相关的定时线程
class MonitorTimerThread : public folly::ScopedEventBaseThread {
 public:
  MonitorTimerThread() : folly::ScopedEventBaseThread("HdfsMonitorTimerThread") {}
  ~MonitorTimerThread() {
    VLOG(5) << "Monitor timer thread delete.";
  }
};

class HdfsMonitorManager {
 public:
  explicit HdfsMonitorManager(uint32_t threads_num);
  virtual ~HdfsMonitorManager();

  virtual folly::EventBase* getEventBase() { return event_thread_->getEventBase(); }
  virtual void addUpdateTask(folly::Func func);
  virtual void monitorSwitch(bool switch_flag);
  virtual bool getSwitch();

 private:
  std::unique_ptr<MonitorTimerThread> event_thread_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> sync_thread_pool_;
  std::atomic<bool> switch_flag_{true};
};

}  // namespace hdfs
