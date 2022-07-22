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
 * @author liubang <it.liubang@gmail.com>
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#include "hdfsmonitor.h"
#include "boost/filesystem.hpp"
#include "common/metrics/metrics.h"

namespace hdfs {

DEFINE_string(call_hdfs_metadata_url,
              "http://127.0.0.1:8080/checksum", "URL to refresh checksum info from laer_control");
DEFINE_bool(call_hdfs_metadata_flag, true, "refresh checksum info from laser_control");
DEFINE_string(hdfs_monitor_matadata, "METADATA", "The name of metadata file, each dir contains one such file");

constexpr static char FILTER_PREFIX_ALL_ITEM[] = "*";
constexpr static char HDFS_MONITOR_MODULE_NAME[] = "hdfs_monitor";
constexpr static char HDFS_MONITOR_WORK_TASK_SIZE_METRIC_NAME[] = "work_thread_pool_task_size";
constexpr static char SERVICE_NAME[] = "hdfs_monitor";
constexpr static char CHECKSUM_INFO_ERROR_COUNT[] = "checksum_info_error_count";
constexpr static char DOWNLOAD_HDFS_FILE_FAIL_COUNT[] = "download_hdfs_file_fail_count";
constexpr static char LASER_METRICS_TAG_DB_NAME[] = "database_name";
constexpr static char LASER_METRICS_TAG_TABLE_NAME[] = "table_name";

HdfsMonitor::HdfsMonitor(const std::string& dir_path, const std::string& dest_path, uint32_t timeval, NotifyCallback cb,
                         std::weak_ptr<HdfsMonitorManager> weak_manager)
    : dir_path_(dir_path), dest_path_(dest_path), timeval_(timeval), cb_(std::move(cb)), weak_manager_(weak_manager) {
  auto manager = weak_manager_.lock();
  if (manager) {
    folly::EventBase* evb = manager->getEventBase();
    timeout_ = folly::AsyncTimeout::make(*evb, [this]() noexcept { watchAndDoMonitor(); });
  }
  // 从dir_path中获取库表，分别对每个库表都建立检测
  std::string database_name;
  std::string table_name;
  getDatabaseNameTableName(dest_path, &database_name, &table_name);
  std::unordered_map<std::string, std::string> tags = {
    {LASER_METRICS_TAG_DB_NAME, database_name},
    {LASER_METRICS_TAG_TABLE_NAME, table_name},
  };
  auto metrics = metrics::Metrics::getInstance();
  checksum_info_error_counter_ = metrics->buildCounter(SERVICE_NAME, CHECKSUM_INFO_ERROR_COUNT, tags);
  download_hdfs_file_fail_counter_ = metrics->buildCounter(SERVICE_NAME, DOWNLOAD_HDFS_FILE_FAIL_COUNT, tags);
  // 默认是匹配所有的文件
  sync_filter_prefixes_.push_back(FILTER_PREFIX_ALL_ITEM);
}

void HdfsMonitor::start() {
  if (!is_start_) {
    watchAndDoMonitor();
    is_start_ = true;
  }
}

void HdfsMonitor::getDatabaseNameTableName(const std::string& dir_path,
                                           std::string* database_name,
                                           std::string* table_name) {
  std::vector<std::string> vec;
  folly::split('/', dir_path, vec);
  if (vec.size() < 3) {
    VLOG(3) << "path is invalid";
    return;
  }
  *database_name = vec[vec.size() - 3];
  *table_name = vec[vec.size() - 2];
}

void HdfsMonitor::addFilter(const std::vector<std::string>& path_prefixes) {
  {
    folly::SpinLockGuard g(spinlock_);
    auto find_all_item = std::find(sync_filter_prefixes_.begin(), sync_filter_prefixes_.end(), FILTER_PREFIX_ALL_ITEM);
    if (find_all_item != sync_filter_prefixes_.end()) {
      sync_filter_prefixes_.erase(find_all_item);
    }

    for (auto& prefix : path_prefixes) {
      if (std::find(sync_filter_prefixes_.begin(), sync_filter_prefixes_.end(), prefix) ==
          sync_filter_prefixes_.end()) {
        sync_filter_prefixes_.push_back(prefix);
      }
    }
  }
  syncMonitorFiles();
}

void HdfsMonitor::delFilter(const std::vector<std::string>& path_prefixes) {
  folly::SpinLockGuard g(spinlock_);
  for (auto& prefix : path_prefixes) {
    auto iter = std::find(sync_filter_prefixes_.begin(), sync_filter_prefixes_.end(), prefix);
    if (iter != sync_filter_prefixes_.end()) {
      sync_filter_prefixes_.erase(iter);
    }
  }
}

void HdfsMonitor::setFilter(const std::vector<std::string>& path_prefixes) {
  {
    folly::SpinLockGuard g(spinlock_);
    sync_filter_prefixes_ = path_prefixes;
  }
  syncMonitorFiles();
}

HdfsMonitor::~HdfsMonitor() { LOG(INFO) << "Delete monitor, dir path: " << dir_path_; }

void HdfsMonitor::destroy() {
  std::shared_ptr<HdfsMonitor> monitor = shared_from_this();
  auto manager = weak_manager_.lock();
  if (manager) {
    manager->getEventBase()->runInEventBaseThread([monitor]() { monitor->timeout_->cancelTimeout(); });
  }
}

void HdfsMonitor::refreshMetadataInfo(std::string metadata_path, ResultStatus *status) {
  metadata_info_.clear();
  if (FLAGS_call_hdfs_metadata_flag) {
    HdfsHttpClient client;
    std::string url = FLAGS_call_hdfs_metadata_url + "?path=" + metadata_path;
    *status = client.refreshChecksumInfo(url, &metadata_info_, this->checksum_info_error_counter_);
  } else {
    *status = hdfscmd_.cat(&metadata_info_, metadata_path);
  }
}

void HdfsMonitor::syncMetaData() {
  folly::SpinLockGuard g(spinlock_);
  if (metadta_sync_status_ == HdfsMetadataSyncStatus::SYNCHING) {
    VLOG(5) << "Sync meta data, current status is syncing, so return.";
    return;
  }

  try {
    std::weak_ptr<HdfsMonitor> weak_monitor = shared_from_this();
    auto manager = weak_manager_.lock();
    if (!manager) {
      return;
    }
    manager->addUpdateTask([weak_monitor]() {
      auto monitor = weak_monitor.lock();
      if (!monitor) {
        LOG(ERROR) << "Monitor has delete";
        return;
      }
      ResultStatus status;
      std::string metadata_path = folly::to<std::string>(monitor->dir_path_, FLAGS_hdfs_monitor_matadata);
      monitor->refreshMetadataInfo(metadata_path, &status);
      {
        folly::SpinLockGuard g(monitor->spinlock_);
        if (status != ResultStatus::RUN_SUCC) {
          LOG(ERROR) << "Get monitor metadata info fail, dir path:" << monitor->dir_path_;
          monitor->metadta_sync_status_ = HdfsMetadataSyncStatus::SYNC_FAILED;
        } else {
          monitor->metadta_sync_status_ = HdfsMetadataSyncStatus::SYNCED;
        }
      }
      if (status == ResultStatus::RUN_SUCC) {
        monitor->syncMonitorFiles();
      }
    });
    metadta_sync_status_ = HdfsMetadataSyncStatus::SYNCHING;
  }
  catch (...) {
    VLOG(5) << "Proccess thread pool queue full.";
  }
}

bool HdfsMonitor::getMonitorFiles(std::unordered_map<std::string, std::string>* monitor_files) {
  std::vector<std::string> list;
  folly::split('\n', metadata_info_, list, true);
  if (list.empty()) {
    VLOG(5) << "Meta data file list is empty, dir path:" << dir_path_;
    return false;
  }

  for (auto& file_meta : list) {
    std::vector<std::string> file_meta_info;
    folly::split('\t', file_meta, file_meta_info, true);
    if (file_meta_info.size() != 2) {
      VLOG(5) << "Meta data file list item format not [filename checksum], info:[" << file_meta << "]";
      return false;
    }

    std::size_t found = file_meta_info[0].find(dir_path_);
    if (found == std::string::npos || found != 0) {
      VLOG(5) << "Meta data file list item filename not contain dir path:" << dir_path_;
      return false;
    }

    std::string filename = file_meta_info[0].substr(dir_path_.size());

    bool has_file = false;
    for (auto& prefix : sync_filter_prefixes_) {
      if (prefix == FILTER_PREFIX_ALL_ITEM) {
        has_file = true;
      }
      std::size_t found = filename.find(prefix);
      if (found != std::string::npos && found == 0) {
        has_file = true;
      }
    }
    if (has_file) {
      (*monitor_files)[file_meta_info[0]] = file_meta_info[1];
    }
  }
  VLOG(5) << "get hdfs files succ, files size is: " << monitor_files->size() << " dir path" << dir_path_;
  return true;
}

void HdfsMonitor::syncMonitorFiles() {
  folly::SpinLockGuard g(spinlock_);
  VLOG(5) << "Hdfs monitor begin to work, dir path:" << dir_path_;
  // 获取 checksum
  std::unordered_map<std::string, std::string> monitor_file_checksum;
  if (!getMonitorFiles(&monitor_file_checksum)) {
    VLOG(5) << "get files by metadata error, dir path:" << dir_path_;
    return;
  }

  for (auto& file_info : monitor_file_checksum) {
    if (file_checksum_.find(file_info.first) != file_checksum_.end() &&
        file_checksum_[file_info.first] == file_info.second) {
      continue;
    }
    try {
      if (sync_files_.find(file_info.first) != sync_files_.end() &&
          sync_files_[file_info.first] == HdfsFileSyncStatus::SYNCHING) {
        continue;
      }
      std::weak_ptr<HdfsMonitor> weak_monitor = shared_from_this();

      auto manager = weak_manager_.lock();
      if (!manager) {
        return;
      }
      manager->addUpdateTask([
        weak_monitor,
        file = file_info.first
      ]() {
         auto monitor = weak_monitor.lock();
         if (!monitor) {
           LOG(ERROR) << "Monitor has delete, not need download file";
           return;
         }
         monitor->downloadFile(file);
       });
      sync_files_[file_info.first] = HdfsFileSyncStatus::SYNCHING;
      file_checksum_[file_info.first] = file_info.second;
    }
    catch (...) {
      VLOG(5) << "Process queue full.";
    }
  }

  std::vector<std::string> deleteKeys;
  for (auto& file_info : file_checksum_) {
    if (monitor_file_checksum.find(file_info.first) == monitor_file_checksum.end()) {
      deleteKeys.push_back(file_info.first);
    }
  }
  for (auto& key : deleteKeys) {
    file_checksum_.erase(key);
  }
}

void HdfsMonitor::updateSyncStatus(const std::string& file, const HdfsFileSyncStatus& status) {
  folly::SpinLockGuard g(spinlock_);
  sync_files_[file] = status;
  std::vector<std::string> synced_files;
  for (auto& file_info : sync_files_) {
    if (file_info.second != HdfsFileSyncStatus::SYNCED) {
      return;
    }
    synced_files.push_back(file_info.first);
  }
  cb_(synced_files);
  sync_files_.clear();
}

bool HdfsMonitor::isSynching() {
  folly::SpinLockGuard g(spinlock_);
  bool has_synching = false;
  for (auto& file : sync_files_) {
    if (file.second == HdfsFileSyncStatus::SYNCHING) {
      has_synching = true;
    }

    if (file.second == HdfsFileSyncStatus::SYNC_FAILED) {
      try {
        std::weak_ptr<HdfsMonitor> weak_monitor = shared_from_this();
        auto manager = weak_manager_.lock();
        if (!manager) {
          return false;
        }
        manager->addUpdateTask([
          weak_monitor,
          filename = file.first
        ]() {
           auto monitor = weak_monitor.lock();
           if (!monitor) {
             LOG(ERROR) << "Monitor has delete";
             return;
           }
           monitor->downloadFile(filename);
         });
        sync_files_[file.first] = HdfsFileSyncStatus::SYNCHING;
      }
      catch (...) {
        sync_files_[file.first] = HdfsFileSyncStatus::SYNC_FAILED;
        VLOG(5) << "Process queue full, sync file:" << file.first << " fail.";
      }
      has_synching = true;
    }
  }
  return has_synching;
}

void HdfsMonitor::downloadFile(const std::string& file) {
  LOG(INFO) << "Start sync file:" << file;

  std::size_t found = file.find(dir_path_);
  if (found == std::string::npos || found != 0) {
    LOG(ERROR) << "Metadata file is invalid, dir_path:" << dir_path_ << " monitor file:" << file;
    return;
  }
  std::string dest_path = folly::to<std::string>(dest_path_, "/", file.substr(dir_path_.size()));
  auto bpath = boost::filesystem::path(dest_path);
  if (boost::filesystem::exists(bpath)) {
    updateSyncStatus(file, HdfsFileSyncStatus::SYNCED);
    VLOG(5) << "Download hdfs file :" << file << " file exists";
    return;
  }

  if (!boost::filesystem::exists(bpath.parent_path())) {
    try {
      boost::filesystem::create_directories(bpath.parent_path());
    }
    catch (...) {
      LOG(ERROR) << "Create dest data dir fail, dir:" << bpath.parent_path();
      updateSyncStatus(file, HdfsFileSyncStatus::SYNC_FAILED);
      return;
    }
  }

  if (hdfs::ResultStatus::RUN_SUCC != hdfscmd_.get(file, dest_path)) {
    LOG(INFO) << "Download hdfs file fail, file:" << file;
    download_hdfs_file_fail_counter_->inc(1);
    updateSyncStatus(file, HdfsFileSyncStatus::SYNC_FAILED);
    return;
  }
  updateSyncStatus(file, HdfsFileSyncStatus::SYNCED);
  VLOG(5) << "Download hdfs file :" << file << " success";
}

//  metadata包含dir和files两个字段，当metadata没有传入files字段时
//  则表明监控该目录下所有文件
void HdfsMonitor::watchAndDoMonitor() {
  bool switch_flag;
  auto weak_manager = weak_manager_.lock();
  if (weak_manager) {
    switch_flag = weak_manager->getSwitch();
  } else {
    switch_flag = true;
  }

  if (!isSynching()) {
    if (switch_flag) {
      syncMetaData();
    } else {
      LOG(INFO) << "Current monitor syncing file switch is stop.";
    }
  } else {
    VLOG(5) << "Current monitor synching file.";
  }

  VLOG(5) << "Restart schedule timeout, dest path:" << dir_path_;

  auto manager = weak_manager_.lock();
  if (manager) {
    manager->getEventBase()->runInEventBaseThread([this]() {
      timeout_->scheduleTimeout(std::chrono::milliseconds(timeval_));
    });
  }
}

HdfsMonitorManager::HdfsMonitorManager(uint32_t thread_nums) {
  event_thread_ = std::make_unique<MonitorTimerThread>();
  sync_thread_pool_ = std::make_shared<folly::CPUThreadPoolExecutor>(
      thread_nums, std::make_shared<folly::NamedThreadFactory>("HdfsMonitorPool"));
  metrics::Metrics::getInstance()->buildGauges(
      HDFS_MONITOR_MODULE_NAME, HDFS_MONITOR_WORK_TASK_SIZE_METRIC_NAME, 1000,
      [this]() { return static_cast<double>(sync_thread_pool_->getTaskQueueSize()); });
}

HdfsMonitorManager::~HdfsMonitorManager() {
  if (sync_thread_pool_) {
    sync_thread_pool_->stop();
  }
}

void HdfsMonitorManager::addUpdateTask(folly::Func func) { sync_thread_pool_->add(std::move(func)); }

void HdfsMonitorManager::monitorSwitch(bool switch_flag) {
  switch_flag_.store(switch_flag);
}

bool HdfsMonitorManager::getSwitch() {
  return switch_flag_.load();
}

}  // namespace hdfs
