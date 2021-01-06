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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#pragma once

#include "folly/SpinLock.h"
#include "folly/Synchronized.h"
#include "folly/ProducerConsumerQueue.h"

#include "common/laser/partition.h"
#include "engine/rocksdb.h"

namespace laser {

class DatabaseMetaInfo {
 public:
  DatabaseMetaInfo() = default;
  virtual ~DatabaseMetaInfo() = default;

  virtual bool init(const rocksdb::Options& options);
  virtual void updateVersion(std::shared_ptr<Partition> partition, const std::string& version);
  virtual const std::string getVersion(std::shared_ptr<Partition> partition);
  virtual void updateDeltaVersions(std::shared_ptr<Partition> partition, const std::vector<std::string>& versions);
  virtual const std::vector<std::string> getDeltaVersions(std::shared_ptr<Partition> partition);
  virtual bool deleteVersion(std::shared_ptr<Partition> partition);

 private:
  std::shared_ptr<RocksDbEngine> meta_db_;
  virtual const std::string generateVersion(std::shared_ptr<Partition> partition);
};

}  // namespace laser
