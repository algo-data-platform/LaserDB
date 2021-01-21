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
 */

#pragma once

#include "folly/Synchronized.h"

#include "common/hdfs/hdfsmonitor.h"
#include "common/laser/partition.h"

namespace laser {

using NotifyPartitionLoadBaseData = folly::Function<void(const std::shared_ptr<Partition>&, const std::string&)>;
using NotifyPartitionLoadDeltaData =
    folly::Function<void(const std::shared_ptr<Partition>&, const std::string&, const std::vector<std::string>&)>;

class TableMonitor {
 public:
  TableMonitor(const std::string& database_name, const std::string& table_name,
               std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_manager);

  virtual ~TableMonitor();

  virtual void init();

  // http 触发更新 某个分区 某个版本 数据
  virtual void updateBase(const std::string& version);

  virtual void updateDelta(const std::string& version, const std::vector<std::string>& delta_versions);

  virtual void addPartition(uint32_t partition_id);

  virtual void removePartition(uint32_t partition_id);

  virtual void subscribeBaseLoad(NotifyPartitionLoadBaseData load_callback) {
    load_base_callback_ = std::move(load_callback);
  }

  virtual void subscribeDeltaLoad(NotifyPartitionLoadDeltaData load_callback) {
    load_delta_callback_ = std::move(load_callback);
  }

 private:
  std::string database_name_;
  std::string table_name_;
  std::string base_version_;
  folly::Synchronized<std::set<uint32_t>> monitor_partitions_;
  NotifyPartitionLoadBaseData load_base_callback_;
  NotifyPartitionLoadDeltaData load_delta_callback_;
  std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor_manager_;
  std::shared_ptr<hdfs::HdfsMonitor> base_monitor_;
  std::shared_ptr<hdfs::HdfsMonitor> delta_monitor_;

  virtual std::shared_ptr<hdfs::HdfsMonitor> createBaseHdfsMonitor();
  virtual std::shared_ptr<hdfs::HdfsMonitor> createDeltaHdfsMonitor();

 protected:
  virtual void updateHdfsBaseData(const std::vector<std::string>& list);
  virtual bool paserUpdateBaseFiles(std::string* base_version, std::vector<uint32_t>* partition_ids,
                                    const std::vector<std::string>& list);
  virtual void updateHdfsDeltaData(const std::vector<std::string>& list);
  virtual bool paserUpdateDeltaFiles(std::vector<std::string>* delta_versions, std::vector<uint32_t>* partition_ids,
                                     const std::vector<std::string>& list);
};

}  // namespace laser
