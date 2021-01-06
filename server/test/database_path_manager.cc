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
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "laser/server/database_manager.h"

class MockDatabaseManager : public laser::DatabaseManager {
 public:
  MockDatabaseManager(const std::string& group_name, uint32_t node_id)
      : laser::DatabaseManager(nullptr, group_name, node_id) {}
};

namespace laser {
DECLARE_string(data_dir);
DECLARE_string(laser_hdfs_data_dir);
}

class DataPathManagerTest : public ::testing::Test {
 public:
  DataPathManagerTest() {}
  virtual ~DataPathManagerTest() = default;
  virtual void TearDown() {
    laser::FLAGS_data_dir = old_data_dir_;
    laser::FLAGS_laser_hdfs_data_dir = old_laser_hdfs_data_dir_;
  }
  virtual void SetUp() {
    // test getDataBase
    EXPECT_EQ(old_data_dir_, laser::DataPathManager::getDataBase());
    laser::FLAGS_data_dir = data_dir_;
    EXPECT_EQ(data_dir_, laser::DataPathManager::getDataBase());

    EXPECT_EQ(old_laser_hdfs_data_dir_, laser::DataPathManager::getHdfsDataBase());
    laser::FLAGS_laser_hdfs_data_dir = laser_hdfs_data_dir_;
    EXPECT_EQ(laser_hdfs_data_dir_, laser::DataPathManager::getHdfsDataBase());
  }

 protected:
  std::string old_data_dir_ = "/tmp/laser";
  std::string data_dir_ = "/unit_test_path";
  std::string old_laser_hdfs_data_dir_ = "/dw_ext/ad/ads_core/laser/dev";
  std::string laser_hdfs_data_dir_ = "/unit_test_hdfs_dir";
  std::string database_name_ = "test";
  std::string table_name_ = "user_info";
  std::string base_version_ = "9735168332383541593";
  std::string delta_version_ = "795101311469057894";
};

TEST_F(DataPathManagerTest, getTableBaseData) {
  std::string ex_path = data_dir_ + "/base/" + database_name_ + "/" + table_name_ + "/";
  EXPECT_EQ(ex_path, laser::DataPathManager::getTableBaseData(data_dir_, database_name_, table_name_));
}

TEST_F(DataPathManagerTest, getTableDeltaData) {
  std::string ex_path = data_dir_ + "/delta/" + database_name_ + "/" + table_name_ + "/" + base_version_ + "/";
  EXPECT_EQ(ex_path, laser::DataPathManager::getTableDeltaData(data_dir_, database_name_, table_name_, base_version_));
}

TEST_F(DataPathManagerTest, getSourceBaseDataFile) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  std::string ex_path =
      data_dir_ + "/source_data/base/" + database_name_ + "/" + table_name_ + "/2/" + base_version_;

  EXPECT_EQ(ex_path, laser::DataPathManager::getSourceBaseDataFile(partition, base_version_));
}

TEST_F(DataPathManagerTest, getSourceDeltaDataFile) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  std::string ex_path = data_dir_ + "/source_data/delta/" + database_name_ + "/" + table_name_ + "/" + base_version_ +
                        "/2/" + delta_version_;

  EXPECT_EQ(ex_path, laser::DataPathManager::getSourceDeltaDataFile(partition, base_version_, delta_version_));
}

TEST_F(DataPathManagerTest, getDeltaLoadDataTempDbPath) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  std::string ex_path =
      data_dir_ + "/temp/delta/" + database_name_ + "/" + table_name_ + "/2/" + base_version_ + "/" + delta_version_;

  EXPECT_EQ(ex_path, laser::DataPathManager::getDeltaLoadDataTempDbPath(partition, base_version_, delta_version_));
}

TEST_F(DataPathManagerTest, getDatabaseDataDir) {
  auto partition = std::make_shared<laser::Partition>(database_name_, table_name_, 2);
  std::string group_name = "test_group";
  uint32_t node_id = 1;
  auto database_manager = std::make_shared<MockDatabaseManager>(group_name, node_id);

  std::string ex_path = data_dir_ + "/data/" + group_name + "/" + folly::to<std::string>(node_id) + "/" +
                        database_name_ + "/" + table_name_ + "/2/" + base_version_;
  EXPECT_EQ(ex_path, laser::DataPathManager::getDatabaseDataDir(database_manager.get(), partition, base_version_));
}
