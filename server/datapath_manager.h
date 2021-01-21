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

#include "common/laser/partition.h"

namespace laser {

inline constexpr static char DATA_PATH_MANAGER_DIR_SPLIT[] = "/";
inline constexpr static char DATA_PATH_MANAGER_DATA_PREFIX[] = "/data";
inline constexpr static char DATA_PATH_MANAGER_TEMP_DATA_PREFIX[] = "/temp";
inline constexpr static char DATA_PATH_MANAGER_SOURCE_DATA_PREFIX[] = "/source_data";
inline constexpr static char DATA_PATH_MANAGER_BASE_DATA_PREFIX[] = "/base";
inline constexpr static char DATA_PATH_MANAGER_DELTA_DATA_PREFIX[] = "/delta";

class DatabaseManager;
class DataPathManager {
 public:
  static const std::string getDataBase();
  static const std::string getHdfsDataBase();

  static const std::string getTableBaseData(const std::string& base_path, const std::string& database_name,
                                            const std::string& table_name);
  static const std::string getTableDeltaData(const std::string& base_path, const std::string& database_name,
                                             const std::string& table_name, const std::string& base_version);

  static const std::string getSourceBaseDataFile(const std::shared_ptr<Partition>& partition,
                                                 const std::string& version);
  static const std::string getSourceDeltaDataFile(const std::shared_ptr<Partition>& partition,
                                                  const std::string& base_version, const std::string& version);
  static const std::string getDeltaLoadDataTempDbPath(const std::shared_ptr<Partition>& partition,
                                                      const std::string& base_version, const std::string& version);
  static const std::string getDatabaseDataDir(DatabaseManager* database_manager,
                                              const std::shared_ptr<Partition>& partition, const std::string& version);
  static const std::string getDatabaseDataReplicatingDir(DatabaseManager* database_manager,
                                                         const std::shared_ptr<Partition>& partition,
                                                         const std::string& version);
  static const std::string getDatabaseDataDeletingDir(DatabaseManager* database_manager,
                                                      const std::shared_ptr<Partition>& partition,
                                                      const std::string& version);
};

}  // namespace laser
