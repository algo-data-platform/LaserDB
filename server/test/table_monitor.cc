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

#include "laser/server/table_monitor.h"
#include "laser/server/datapath_manager.h"
#include "common/hdfs/hdfsmonitor.h"

class MockHdfsMonitor : public hdfs::HdfsMonitor {
 public:
  explicit MockHdfsMonitor(const std::string& dir_path, const std::string& dest_path, uint32_t timeval,
                           hdfs::NotifyCallback cb, std::shared_ptr<hdfs::HdfsMonitorManager> manager)
      : hdfs::HdfsMonitor(dir_path, dest_path, timeval, std::move(cb), manager) {}
  MOCK_METHOD0(start, void());
  MOCK_METHOD1(addFilter, void(const std::vector<std::string>&));
  MOCK_METHOD1(setFilter, void(const std::vector<std::string>&));
  MOCK_METHOD1(delFilter, void(const std::vector<std::string>&));
};

class MockTableMonitor : public laser::TableMonitor {
 public:
  explicit MockTableMonitor(std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor, const std::string& database_name,
                            const std::string& table_name)
      : laser::TableMonitor(database_name, table_name, hdfs_monitor) {}
  MOCK_METHOD0(createBaseHdfsMonitor, std::shared_ptr<hdfs::HdfsMonitor>());
  MOCK_METHOD0(createDeltaHdfsMonitor, std::shared_ptr<hdfs::HdfsMonitor>());
  bool paserUpdateBaseFiles(std::string* base_version, std::vector<uint32_t>* partition_ids,
                            const std::vector<std::string>& list) override {
    return laser::TableMonitor::paserUpdateBaseFiles(base_version, partition_ids, list);
  }
  bool paserUpdateDeltaFiles(std::vector<std::string>* delta_versions, std::vector<uint32_t>* partition_ids,
                             const std::vector<std::string>& list) override {
    return laser::TableMonitor::paserUpdateDeltaFiles(delta_versions, partition_ids, list);
  }
  void updateHdfsBaseData(const std::vector<std::string>& list) override {
    return laser::TableMonitor::updateHdfsBaseData(list);
  }
  void updateHdfsDeltaData(const std::vector<std::string>& list) override {
    return laser::TableMonitor::updateHdfsDeltaData(list);
  }
};

class TableMonitorTest : public ::testing::Test {
 public:
  TableMonitorTest() {
    folly::SingletonVault::singleton()->registrationComplete();
    hdfs_monitor_manager_ = std::make_shared<hdfs::HdfsMonitorManager>(1);
  }
  virtual ~TableMonitorTest() = default;
  virtual void TearDown() {
    delta_hdfs_monitor_->destroy();
  }
  virtual void SetUp() {
    hdfs_monitor_ =
        std::make_shared<MockHdfsMonitor>(dir_path_, dest_path_, 10, [this](auto&) {}, hdfs_monitor_manager_);
    delta_hdfs_monitor_ =
        std::make_shared<MockHdfsMonitor>(dir_path_, dest_path_, 10, [this](auto&) {}, hdfs_monitor_manager_);
    table_monitor_ = std::make_shared<MockTableMonitor>(hdfs_monitor_manager_, database_name_, table_name_);
    EXPECT_CALL(*table_monitor_, createBaseHdfsMonitor()).Times(1).WillOnce(::testing::Return(hdfs_monitor_));
    table_monitor_->init();
  }

 protected:
  std::shared_ptr<MockTableMonitor> table_monitor_;
  std::shared_ptr<MockHdfsMonitor> hdfs_monitor_;
  std::shared_ptr<MockHdfsMonitor> delta_hdfs_monitor_;
  std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor_manager_;
  std::string dir_path_ = "/bar";
  std::string dest_path_ = "/foo";
  std::string database_name_ = "test";
  std::string table_name_ = "user_info";

  void updateHdfsBaseData(const std::string& base_version) {
    EXPECT_CALL(*hdfs_monitor_, start()).Times(2);
    EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("1/"))).Times(1);
    EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("2/"))).Times(1);
    table_monitor_->addPartition(1);
    table_monitor_->addPartition(2);

    std::string base_data_path = laser::DataPathManager::getTableBaseData(laser::DataPathManager::getHdfsDataBase(),
                                                                          database_name_, table_name_);
    std::vector<std::string> base_list(
        {base_data_path + "/1/" + base_version, base_data_path + "/2/" + base_version});
    EXPECT_CALL(*table_monitor_, createDeltaHdfsMonitor()).Times(1).WillOnce(::testing::Return(delta_hdfs_monitor_));
    EXPECT_CALL(*delta_hdfs_monitor_, start()).Times(1);
    EXPECT_CALL(*delta_hdfs_monitor_, setFilter(::testing::ElementsAre("1/", "2/"))).Times(1);

    table_monitor_->updateHdfsBaseData(base_list);
  }
};

TEST_F(TableMonitorTest, table_monitor) {
  std::string update_version = "foo_version";

  int count = 0;
  table_monitor_->subscribeBaseLoad([&count](const std::shared_ptr<laser::Partition>& partition,
                                             const std::string& version) { count++; });
  table_monitor_->updateBase(update_version);
  EXPECT_EQ(0, count);

  EXPECT_CALL(*hdfs_monitor_, start()).Times(2);
  EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("1/"))).Times(1);
  EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("2/"))).Times(1);
  int count_1 = 0;
  table_monitor_->addPartition(1);
  table_monitor_->addPartition(2);
  std::set<uint32_t> callback_patitions;
  table_monitor_->subscribeBaseLoad([&count_1, &callback_patitions, &update_version](
      const std::shared_ptr<laser::Partition>& partition, const std::string& version) {
    EXPECT_EQ(update_version, version);
    callback_patitions.insert(partition->getPartitionId());
    count_1++;
  });
  table_monitor_->updateBase(update_version);
  EXPECT_EQ(2, count_1);
  EXPECT_THAT(std::set<uint32_t>({1, 2}), ::testing::ContainerEq(callback_patitions));

  EXPECT_CALL(*hdfs_monitor_, start()).Times(1);
  EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("3/"))).Times(1);
  EXPECT_CALL(*hdfs_monitor_, delFilter(::testing::Contains("2/"))).Times(1);
  int count_2 = 0;
  table_monitor_->addPartition(3);
  table_monitor_->removePartition(2);
  callback_patitions.clear();
  table_monitor_->subscribeBaseLoad([&count_2, &callback_patitions, &update_version](
      const std::shared_ptr<laser::Partition>& partition, const std::string& version) {
    EXPECT_EQ(update_version, version);
    callback_patitions.insert(partition->getPartitionId());
    count_2++;
  });
  table_monitor_->updateBase(update_version);
  EXPECT_EQ(2, count_2);
  EXPECT_THAT(std::set<uint32_t>({1, 3}), ::testing::ContainerEq(callback_patitions));
}

TEST_F(TableMonitorTest, paserUpdateBaseFiles) {
  std::string base_version;
  std::vector<uint32_t> partition_ids;
  std::string data_path =
      laser::DataPathManager::getTableBaseData(laser::DataPathManager::getHdfsDataBase(), database_name_, table_name_);

  std::vector<std::string> invalid_prefix_file_list({"/invalid/aa", "/invalid/bb"});
  EXPECT_FALSE(table_monitor_->paserUpdateBaseFiles(&base_version, &partition_ids, invalid_prefix_file_list));

  std::vector<std::string> invalid_file_format_list({data_path + "/aa"});
  EXPECT_FALSE(table_monitor_->paserUpdateBaseFiles(&base_version, &partition_ids, invalid_file_format_list));

  std::vector<std::string> invalid_partid_format_list({data_path + "/invalid_partition_id/foo_version"});
  EXPECT_FALSE(table_monitor_->paserUpdateBaseFiles(&base_version, &partition_ids, invalid_partid_format_list));

  std::vector<std::string> multi_version_list({data_path + "/2/version1", data_path + "/3/version2"});
  EXPECT_FALSE(table_monitor_->paserUpdateBaseFiles(&base_version, &partition_ids, multi_version_list));

  std::string test_version = "test_version";
  std::vector<std::string> list({data_path + "/1/" + test_version, data_path + "/2/" + test_version});
  EXPECT_TRUE(table_monitor_->paserUpdateBaseFiles(&base_version, &partition_ids, list));
  EXPECT_EQ(base_version, "test_version");
  EXPECT_THAT(std::vector<uint32_t>({1, 2}), ::testing::ContainerEq(partition_ids));
}

TEST_F(TableMonitorTest, paserUpdateDeltaFiles) {
  // 创建 base
  std::string base_version = "base_test_version";
  updateHdfsBaseData(base_version);

  std::vector<uint32_t> partition_ids;
  std::vector<std::string> delta_versions;
  std::string data_path = laser::DataPathManager::getTableDeltaData(laser::DataPathManager::getHdfsDataBase(),
                                                                    database_name_, table_name_, base_version);
  LOG(INFO) << "datapath:" << data_path;

  std::vector<std::string> invalid_prefix_file_list({"/invalid/aa", "/invalid/bb"});
  EXPECT_FALSE(table_monitor_->paserUpdateDeltaFiles(&delta_versions, &partition_ids, invalid_prefix_file_list));

  std::vector<std::string> invalid_file_format_list({data_path + "/aa"});
  EXPECT_FALSE(table_monitor_->paserUpdateDeltaFiles(&delta_versions, &partition_ids, invalid_file_format_list));

  std::vector<std::string> invalid_partid_format_list({data_path + "/invalid_partition_id/foo_version"});
  EXPECT_FALSE(table_monitor_->paserUpdateDeltaFiles(&delta_versions, &partition_ids, invalid_partid_format_list));

  std::string test_version = "test_version";
  std::string test_version1 = "test_version1";
  std::vector<std::string> list({data_path + "1/" + test_version, data_path + "/2/" + test_version1});
  EXPECT_TRUE(table_monitor_->paserUpdateDeltaFiles(&delta_versions, &partition_ids, list));
  EXPECT_THAT(std::vector<uint32_t>({1, 2}), ::testing::ContainerEq(partition_ids));
  EXPECT_THAT(std::vector<std::string>({test_version, test_version1}), ::testing::ContainerEq(delta_versions));
}

TEST_F(TableMonitorTest, updateHdfsBaseDataFail) {
  EXPECT_CALL(*hdfs_monitor_, start()).Times(2);
  EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("1/"))).Times(1);
  EXPECT_CALL(*hdfs_monitor_, addFilter(::testing::Contains("2/"))).Times(1);
  table_monitor_->addPartition(1);
  table_monitor_->addPartition(2);

  std::string base_version = "test_version";

  int count = 0;
  table_monitor_->subscribeBaseLoad([&count](const std::shared_ptr<laser::Partition>& partition,
                                             const std::string& version) { count++; });
  EXPECT_CALL(*table_monitor_, createDeltaHdfsMonitor()).Times(1).WillOnce(::testing::Return(delta_hdfs_monitor_));
  EXPECT_CALL(*delta_hdfs_monitor_, start()).Times(1);
  EXPECT_CALL(*delta_hdfs_monitor_, setFilter(::testing::ElementsAre("1/", "2/"))).Times(1);

  std::string base_data_path =
      laser::DataPathManager::getTableBaseData(laser::DataPathManager::getHdfsDataBase(), database_name_, table_name_);
  std::vector<std::string> base_list(
      {base_data_path + "/1/" + base_version, base_data_path + "/2/" + base_version});
  table_monitor_->updateHdfsBaseData(base_list);
  EXPECT_EQ(2, count);

  int count_1 = 0;
  table_monitor_->subscribeBaseLoad([&count_1](const std::shared_ptr<laser::Partition>& partition,
                                               const std::string& version) { count_1++; });

  std::vector<std::string> invalid_list({base_data_path + "/1", base_data_path + "/2"});
  table_monitor_->updateHdfsBaseData(invalid_list);
  EXPECT_EQ(0, count_1);
}

TEST_F(TableMonitorTest, updateHdfsDeltaData) {
  // 创建 base
  std::string base_version = "base_test_version";
  updateHdfsBaseData(base_version);

  std::vector<uint32_t> partition_ids;
  std::vector<std::string> delta_versions;
  std::string data_path = laser::DataPathManager::getTableDeltaData(laser::DataPathManager::getHdfsDataBase(),
                                                                    database_name_, table_name_, base_version);

  std::vector<std::string> invalid_prefix_file_list({"/invalid/aa", "/invalid/bb"});
  int count = 0;
  table_monitor_->subscribeDeltaLoad([&count](const std::shared_ptr<laser::Partition>& partition, const std::string&,
                                              const std::vector<std::string>&) { count++; });
  table_monitor_->updateHdfsDeltaData(invalid_prefix_file_list);
  EXPECT_EQ(0, count);

  std::string test_version = "test_version";
  std::string test_version1 = "test_version1";
  std::vector<std::string> list({data_path + "1/" + test_version, data_path + "/2/" + test_version1});
  int count_1 = 0;
  table_monitor_->subscribeDeltaLoad([&count_1](const std::shared_ptr<laser::Partition>& partition, const std::string&,
                                                const std::vector<std::string>&) { count_1++; });
  table_monitor_->updateHdfsDeltaData(list);
  EXPECT_EQ(2, count_1);
}
