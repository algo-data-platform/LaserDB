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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "boost/filesystem.hpp"

#include "laser/server/database_meta_info.h"

namespace laser {
DECLARE_string(data_dir);
}

class DatabaseMetaInfoTest : public ::testing::Test {
 public:
  DatabaseMetaInfoTest() {
    folly::SingletonVault::singleton()->registrationComplete();
  }
  virtual ~DatabaseMetaInfoTest() = default;
  virtual void TearDown() {
    boost::filesystem::path bpath(data_dir_);
    if (boost::filesystem::exists(data_dir_)) {
      boost::filesystem::remove_all(data_dir_);
    }
  }
  virtual void SetUp() {
    // test getDataBase
    laser::FLAGS_data_dir = data_dir_;
    database_meta_info_ = std::make_shared<laser::DatabaseMetaInfo>();
    init();
  }

 protected:
  std::string data_dir_ = "/tmp/database_meta_info_unit_test";
  std::string database_name_ = "test";
  std::string table_name_ = "user_info";
  std::string base_version_ = "9735168332383541593";
  std::shared_ptr<laser::DatabaseMetaInfo> database_meta_info_;

  void init() {
    rocksdb::Options options;
    options.create_if_missing = true;
    EXPECT_TRUE(database_meta_info_->init(options));
  }
};

TEST_F(DatabaseMetaInfoTest, updateVersion) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  database_meta_info_->updateVersion(partition, base_version_);
  EXPECT_EQ(base_version_, database_meta_info_->getVersion(partition));
}

TEST_F(DatabaseMetaInfoTest, deltaVersions) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  std::vector<std::string> delta_versions({"9735168332383541593", "9735168332383541594", "9735168332383541595"});
  database_meta_info_->updateDeltaVersions(partition, delta_versions);
  auto& versions = database_meta_info_->getDeltaVersions(partition);
  EXPECT_THAT(delta_versions, ::testing::ContainerEq(versions));
}
