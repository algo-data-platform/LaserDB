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

#include "datapath_manager.h"
#include "database_manager.h"

namespace laser {

DEFINE_string(data_dir, "/tmp/laser", "Laser data base path");
DEFINE_string(laser_hdfs_data_dir, "/dw_ext/ad/ads_core/laser/dev", "Laser hdfs data base path");

const std::string DataPathManager::getDataBase() { return FLAGS_data_dir; }

const std::string DataPathManager::getHdfsDataBase() { return FLAGS_laser_hdfs_data_dir; }

const std::string DataPathManager::getTableBaseData(const std::string& base_path, const std::string& database_name,
                                                    const std::string& table_name) {
  return folly::to<std::string>(base_path, DATA_PATH_MANAGER_BASE_DATA_PREFIX, DATA_PATH_MANAGER_DIR_SPLIT,
                                database_name, DATA_PATH_MANAGER_DIR_SPLIT, table_name, DATA_PATH_MANAGER_DIR_SPLIT);
}

const std::string DataPathManager::getTableDeltaData(const std::string& base_path, const std::string& database_name,
                                                     const std::string& table_name, const std::string& base_version) {
  return folly::to<std::string>(base_path, DATA_PATH_MANAGER_DELTA_DATA_PREFIX, DATA_PATH_MANAGER_DIR_SPLIT,
                                database_name, DATA_PATH_MANAGER_DIR_SPLIT, table_name, DATA_PATH_MANAGER_DIR_SPLIT,
                                base_version, DATA_PATH_MANAGER_DIR_SPLIT);
}

const std::string DataPathManager::getSourceBaseDataFile(const std::shared_ptr<Partition>& partition,
                                                         const std::string& version) {
  return folly::to<std::string>(FLAGS_data_dir, DATA_PATH_MANAGER_SOURCE_DATA_PREFIX,
                                DATA_PATH_MANAGER_BASE_DATA_PREFIX, DATA_PATH_MANAGER_DIR_SPLIT,
                                partition->getDatabaseName(), DATA_PATH_MANAGER_DIR_SPLIT, partition->getTableName(),
                                DATA_PATH_MANAGER_DIR_SPLIT, partition->getPartitionId(), DATA_PATH_MANAGER_DIR_SPLIT,
                                version);
}

const std::string DataPathManager::getSourceDeltaDataFile(const std::shared_ptr<Partition>& partition,
                                                          const std::string& base_version, const std::string& version) {
  return folly::to<std::string>(
      FLAGS_data_dir, DATA_PATH_MANAGER_SOURCE_DATA_PREFIX, DATA_PATH_MANAGER_DELTA_DATA_PREFIX,
      DATA_PATH_MANAGER_DIR_SPLIT, partition->getDatabaseName(), DATA_PATH_MANAGER_DIR_SPLIT, partition->getTableName(),
      DATA_PATH_MANAGER_DIR_SPLIT, base_version, DATA_PATH_MANAGER_DIR_SPLIT, partition->getPartitionId(),
      DATA_PATH_MANAGER_DIR_SPLIT, version);
}

const std::string DataPathManager::getDeltaLoadDataTempDbPath(const std::shared_ptr<Partition>& partition,
                                                              const std::string& base_version,
                                                              const std::string& version) {
  return folly::to<std::string>(FLAGS_data_dir, DATA_PATH_MANAGER_TEMP_DATA_PREFIX, DATA_PATH_MANAGER_DELTA_DATA_PREFIX,
                                DATA_PATH_MANAGER_DIR_SPLIT, partition->getDatabaseName(), DATA_PATH_MANAGER_DIR_SPLIT,
                                partition->getTableName(), DATA_PATH_MANAGER_DIR_SPLIT, partition->getPartitionId(),
                                DATA_PATH_MANAGER_DIR_SPLIT, base_version, DATA_PATH_MANAGER_DIR_SPLIT, version);
}

const std::string DataPathManager::getDatabaseDataDir(DatabaseManager* database_manager,
                                                      const std::shared_ptr<Partition>& partition,
                                                      const std::string& version) {
  return folly::to<std::string>(
      FLAGS_data_dir, DATA_PATH_MANAGER_DATA_PREFIX, DATA_PATH_MANAGER_DIR_SPLIT, database_manager->getGroupName(),
      DATA_PATH_MANAGER_DIR_SPLIT, database_manager->getNodeId(), DATA_PATH_MANAGER_DIR_SPLIT,
      partition->getDatabaseName(), DATA_PATH_MANAGER_DIR_SPLIT, partition->getTableName(), DATA_PATH_MANAGER_DIR_SPLIT,
      partition->getPartitionId(), DATA_PATH_MANAGER_DIR_SPLIT, version);
}

const std::string DataPathManager::getDatabaseDataReplicatingDir(DatabaseManager* database_manager,
                                                                 const std::shared_ptr<Partition>& partition,
                                                                 const std::string& version) {
  return folly::to<std::string>(getDatabaseDataDir(database_manager, partition, version), "_replicating");
}

const std::string DataPathManager::getDatabaseDataDeletingDir(DatabaseManager* database_manager,
                                                              const std::shared_ptr<Partition>& partition,
                                                              const std::string& version) {
  return folly::to<std::string>(getDatabaseDataDir(database_manager, partition, version), "_deleting");
}

}  // namespace laser
