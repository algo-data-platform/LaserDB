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

#include "common/service_router/router.h"

#include "datapath_manager.h"
#include "table_monitor.h"

namespace laser {

DEFINE_int32(hdfs_monitor_interval, 3000, "Laser sync hdfs monitor interval");

TableMonitor::TableMonitor(const std::string& database_name, const std::string& table_name,
                           std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_manager)
    : database_name_(database_name), table_name_(table_name), hdfs_monitor_manager_(hdfs_manager) {}

TableMonitor::~TableMonitor() {
  if (base_monitor_) {
    base_monitor_->destroy();
  }
  if (delta_monitor_) {
    delta_monitor_->destroy();
  }
}

void TableMonitor::init() { base_monitor_ = createBaseHdfsMonitor(); }

std::shared_ptr<hdfs::HdfsMonitor> TableMonitor::createBaseHdfsMonitor() {
  std::string hdfs_data_path =
      DataPathManager::getTableBaseData(DataPathManager::getHdfsDataBase(), database_name_, table_name_);
  std::string base_path = folly::to<std::string>(DataPathManager::getDataBase(), DATA_PATH_MANAGER_DIR_SPLIT,
                                                 DATA_PATH_MANAGER_SOURCE_DATA_PREFIX);
  std::string data_path = DataPathManager::getTableBaseData(base_path, database_name_, table_name_);
  return std::make_shared<hdfs::HdfsMonitor>(hdfs_data_path, data_path, FLAGS_hdfs_monitor_interval,
                                             [this](auto& list) { updateHdfsBaseData(list); }, hdfs_monitor_manager_);
}

std::shared_ptr<hdfs::HdfsMonitor> TableMonitor::createDeltaHdfsMonitor() {
  std::string hdfs_data_path = DataPathManager::getTableDeltaData(DataPathManager::getHdfsDataBase(), database_name_,
                                                                  table_name_, base_version_);
  std::string base_path = folly::to<std::string>(DataPathManager::getDataBase(), DATA_PATH_MANAGER_DIR_SPLIT,
                                                 DATA_PATH_MANAGER_SOURCE_DATA_PREFIX);
  std::string data_path = DataPathManager::getTableDeltaData(base_path, database_name_, table_name_, base_version_);
  return std::make_shared<hdfs::HdfsMonitor>(hdfs_data_path, data_path, FLAGS_hdfs_monitor_interval,
                                             [this](auto& list) { updateHdfsDeltaData(list); }, hdfs_monitor_manager_);
}

void TableMonitor::updateHdfsBaseData(const std::vector<std::string>& list) {
  std::string base_version;
  std::vector<uint32_t> partition_ids;
  if (!paserUpdateBaseFiles(&base_version, &partition_ids, list)) {
    return;
  }

  updateBase(base_version);
  if (base_version_ != base_version) {  // 当 base version 发生变更，重新监控新的对应的 delta 目录
    base_version_ = base_version;
    if (delta_monitor_) {
      delta_monitor_->destroy();
    }
    delta_monitor_ = createDeltaHdfsMonitor();
    std::vector<std::string> filter_prefixes;
    monitor_partitions_.withRLock([&filter_prefixes](auto& partition_ids) {
      for (auto& partition_id : partition_ids) {
        filter_prefixes.push_back(folly::to<std::string>(partition_id, DATA_PATH_MANAGER_DIR_SPLIT));
      }
    });
    delta_monitor_->setFilter(filter_prefixes);
    delta_monitor_->start();
  }
}

void TableMonitor::updateHdfsDeltaData(const std::vector<std::string>& list) {
  std::vector<std::string> delta_versions;
  std::vector<uint32_t> partition_ids;
  if (!paserUpdateDeltaFiles(&delta_versions, &partition_ids, list)) {
    return;
  }

  updateDelta(base_version_, delta_versions);
}

bool TableMonitor::paserUpdateBaseFiles(std::string* base_version, std::vector<uint32_t>* partition_ids,
                                        const std::vector<std::string>& list) {
  base_version->clear();
  partition_ids->clear();
  std::string hdfs_data_path =
      DataPathManager::getTableBaseData(DataPathManager::getHdfsDataBase(), database_name_, table_name_);
  for (auto& file : list) {
    std::size_t found = file.find(hdfs_data_path);
    if (found == std::string::npos || found != 0) {
      LOG(ERROR) << "Monitor update file is invalid, dir_path:" << hdfs_data_path << " monitor file:" << file;
      return false;
    }
    std::string partition_path = file.substr(hdfs_data_path.size());
    std::vector<std::string> partition_info;
    folly::split(DATA_PATH_MANAGER_DIR_SPLIT, partition_path, partition_info, true);
    if (partition_info.size() != 2) {
      LOG(ERROR) << "Monitor update file is invalid, partition path is :" << partition_path
                 << "  file format is [base_path/partition_id/version]";
      return false;
    }
    auto partition_id_opt = folly::tryTo<uint32_t>(partition_info[0]);
    if (!partition_id_opt.hasValue()) {
      LOG(ERROR) << "Monitor update file is invalid, partition path is :" << partition_path
                 << " partition id must is int";
      return false;
    }

    if (!base_version->empty() && *base_version != partition_info[1]) {
      LOG(ERROR) << "Monitor update file is invalid, exists multi version:" << *base_version << " and "
                 << partition_info[1];
      return false;
    }

    *base_version = partition_info[1];
    partition_ids->push_back(partition_id_opt.value());
  }

  return true;
}

bool TableMonitor::paserUpdateDeltaFiles(std::vector<std::string>* delta_versions, std::vector<uint32_t>* partition_ids,
                                         const std::vector<std::string>& list) {
  std::string hdfs_data_path = DataPathManager::getTableDeltaData(DataPathManager::getHdfsDataBase(), database_name_,
                                                                  table_name_, base_version_);
  for (auto& file : list) {
    std::size_t found = file.find(hdfs_data_path);
    if (found == std::string::npos || found != 0) {
      LOG(ERROR) << "Monitor update file is invalid, dir_path:" << hdfs_data_path << " monitor file:" << file;
      return false;
    }
    std::string partition_path = file.substr(hdfs_data_path.size());
    std::vector<std::string> partition_info;
    folly::split(DATA_PATH_MANAGER_DIR_SPLIT, partition_path, partition_info, true);
    if (partition_info.size() != 2) {
      LOG(ERROR) << "Monitor update file is invalid, partition path is :" << partition_path;
      return false;
    }
    auto partition_id_opt = folly::tryTo<uint32_t>(partition_info[0]);
    if (!partition_id_opt.hasValue()) {
      LOG(ERROR) << "Monitor update file is invalid, partition path is :" << partition_path;
      return false;
    }

    if (std::find(delta_versions->begin(), delta_versions->end(), partition_info[1]) == delta_versions->end()) {
      delta_versions->push_back(partition_info[1]);
    }
    if (std::find(partition_ids->begin(), partition_ids->end(), partition_id_opt.value()) == partition_ids->end()) {
      partition_ids->push_back(partition_id_opt.value());
    }
  }

  return true;
}

void TableMonitor::addPartition(uint32_t partition_id) {
  monitor_partitions_.withWLock([partition_id, this](auto& partitions) { partitions.insert(partition_id); });

  std::string prefix = folly::to<std::string>(partition_id, DATA_PATH_MANAGER_DIR_SPLIT);
  base_monitor_->addFilter({prefix});
  base_monitor_->start();
  if (delta_monitor_) {
    delta_monitor_->addFilter({prefix});
  }
}

void TableMonitor::removePartition(uint32_t partition_id) {
  monitor_partitions_.withWLock([partition_id, this](auto& partitions) {
    auto iter = std::find(partitions.begin(), partitions.end(), partition_id);
    if (iter != partitions.end()) {
      partitions.erase(iter);
    }
  });
  std::string prefix = folly::to<std::string>(partition_id, DATA_PATH_MANAGER_DIR_SPLIT);
  base_monitor_->delFilter({prefix});
  if (delta_monitor_) {
    delta_monitor_->delFilter({prefix});
  }
}

void TableMonitor::updateBase(const std::string& version) {
  monitor_partitions_.withRLock([&version, this](auto& partitions) {
    for (uint32_t partition_id : partitions) {
      if (load_base_callback_) {
        load_base_callback_(std::make_shared<Partition>(database_name_, table_name_, partition_id), version);
      }
    }
  });
}

void TableMonitor::updateDelta(const std::string& version, const std::vector<std::string>& delta_versions) {
  monitor_partitions_.withRLock([&version, &delta_versions, this](auto& partitions) {
    for (uint32_t partition_id : partitions) {
      if (load_delta_callback_) {
        load_delta_callback_(std::make_shared<Partition>(database_name_, table_name_, partition_id), version,
                             delta_versions);
      }
    }
  });
}

}  // namespace laser
