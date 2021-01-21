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

#include "loader_source_data.h"
#include "format.h"
#include "rocksdb_config_factory.h"
#include "laser/server/engine/rocksdb.h"

namespace laser {

class GenerateSst {
 public:
  GenerateSst(const std::string& temp_db_path, const std::string& sst_file_path, const rocksdb::Options& options);
  ~GenerateSst() = default;
  bool open();
  bool insertData(const laser::LoaderSourceData& data);
  bool batchInsertRawString(const std::vector<std::shared_ptr<laser::LoaderSourceData>>& data);
  bool dumpSst();

 private:
  std::string sst_file_path_;
  std::shared_ptr<laser::RocksDbEngine> db_;
  bool insertRawString(const laser::LoaderSourceData& data);
  bool insertCounter(const laser::LoaderSourceData& data);
  bool insertMap(const laser::LoaderSourceData& data);
  bool insertSet(const laser::LoaderSourceData& data);
  bool insertList(const laser::LoaderSourceData& data);
};

}  // namespace laser
