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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "common/service_router/router.h"
#include "common/laser/partition.h"

TEST(Partition, set_test) {
  std::set<std::shared_ptr<laser::Partition>, laser::PartitionCompare> data;
  EXPECT_EQ(0, data.size());

  data.insert(std::make_shared<laser::Partition>("test", "table_test", 1));
  data.insert(std::make_shared<laser::Partition>("test", "table_test", 2));
  EXPECT_EQ(2, data.size());
  // 重复的值
  data.insert(std::make_shared<laser::Partition>("test", "table_test", 2));
  EXPECT_EQ(2, data.size());
}

TEST(Partition, set_difference) {
  std::set<std::shared_ptr<laser::Partition>, laser::PartitionCompare> one;
  one.insert(std::make_shared<laser::Partition>("test", "table_test", 1));
  one.insert(std::make_shared<laser::Partition>("test", "table_test", 2));
  one.insert(std::make_shared<laser::Partition>("test", "table_test", 4));
  one.insert(std::make_shared<laser::Partition>("test", "table_test", 5));
  one.insert(std::make_shared<laser::Partition>("test", "table_test", 6));

  std::set<std::shared_ptr<laser::Partition>, laser::PartitionCompare> two;
  two.insert(std::make_shared<laser::Partition>("test", "table_test", 3));
  two.insert(std::make_shared<laser::Partition>("test", "table_test", 4));
  two.insert(std::make_shared<laser::Partition>("test", "table_test", 6));

  std::set<std::shared_ptr<laser::Partition>, laser::PartitionCompare> one_diff;
  std::set_difference(one.begin(), one.end(), two.begin(), two.end(), std::inserter(one_diff, one_diff.begin()),
                      [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });
  EXPECT_EQ(3, one_diff.size());

  std::set<std::shared_ptr<laser::Partition>, laser::PartitionCompare> two_diff;
  std::set_difference(two.begin(), two.end(), one.begin(), one.end(), std::inserter(two_diff, two_diff.begin()),
                      [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });
  EXPECT_EQ(1, two_diff.size());
}

class MockConfigManager : public laser::ConfigManager {
 public:
  explicit MockConfigManager(const std::shared_ptr<service_router::Router> router) : laser::ConfigManager(router) {}
  MOCK_METHOD2(getTableSchema,
               folly::Optional<std::shared_ptr<laser::TableSchema>>(const std::string&, const std::string&));
  MOCK_METHOD0(getShardNumber, folly::Optional<uint32_t>());
  void subscribe(const std::string&, uint32_t, laser::NotifyDatabaseAndClusterUpdate callback) override {
    update_callback_ = std::move(callback);
  }

  void update(const laser::NodeShardList& nodes,
              const std::unordered_map<uint64_t, std::shared_ptr<laser::TableSchema>>& list) {
    update_callback_(nodes, list);
  }

 private:
  laser::NotifyDatabaseAndClusterUpdate update_callback_;
};

class PartitionManagerTest : public ::testing::Test {
 public:
  PartitionManagerTest() { config_ = std::make_shared<MockConfigManager>(nullptr); }
  virtual ~PartitionManagerTest() = default;
  virtual void TearDown() {}
  virtual void SetUp() {
    partition_manager_ = std::make_shared<laser::PartitionManager>(config_, group_name_, node_id_);
  }

 protected:
  std::shared_ptr<MockConfigManager> config_;
  std::shared_ptr<laser::PartitionManager> partition_manager_;
  std::string database_name_ = "test";
  std::string table_name_ = "user";
  std::string group_name_ = "test_group";
  uint32_t node_id_ = 1;
};

TEST_F(PartitionManagerTest, getPartitionId) {
  std::shared_ptr<laser::TableSchema> table_ptr = std::make_shared<laser::TableSchema>();
  table_ptr->setPartitionNumber(10);
  folly::Optional<std::shared_ptr<laser::TableSchema>> table_opt_ptr = table_ptr;

  EXPECT_CALL(*config_, getTableSchema(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(table_ptr))
      .WillOnce(::testing::Return(folly::none));

  laser::LaserKeyFormat format_key({"foo"}, {"bar"});

  auto partition_id = laser::PartitionManager::getPartitionId(config_, database_name_, table_name_, format_key);
  EXPECT_EQ(5, partition_id.value());
  auto partition_none = laser::PartitionManager::getPartitionId(config_, database_name_, table_name_, format_key);
  EXPECT_FALSE(partition_none.hasValue());

  EXPECT_EQ(5, laser::PartitionManager::getPartitionId(database_name_, table_name_, format_key, 10));
}

TEST_F(PartitionManagerTest, getShardId) {
  EXPECT_CALL(*config_, getShardNumber()).Times(2).WillOnce(::testing::Return(folly::Optional<uint32_t>(10))).WillOnce(
      ::testing::Return(folly::none));

  std::shared_ptr<laser::Partition> partition = std::make_shared<laser::Partition>(database_name_, table_name_, 12);

  auto shard_id = laser::PartitionManager::getShardId(partition, config_);
  EXPECT_EQ(3, shard_id.value());
  auto shard_none = laser::PartitionManager::getShardId(partition, config_);
  EXPECT_FALSE(shard_none.hasValue());
}

void expectPartitionPtrSet(const laser::PartitionPtrSet& p1, const laser::PartitionPtrSet& p2) {
  EXPECT_EQ(p1.size(), p2.size());
  laser::PartitionPtrSet diff;
  std::set_difference(p1.begin(), p1.end(), p2.begin(), p2.end(), std::inserter(diff, diff.begin()),
                      [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });
  EXPECT_EQ(0, diff.size());
  std::set_difference(p2.begin(), p2.end(), p1.begin(), p1.end(), std::inserter(diff, diff.begin()),
                      [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });
  EXPECT_EQ(0, diff.size());
}

TEST_F(PartitionManagerTest, updateShardList) {
  laser::NodeShardList node_shard_list;
  node_shard_list.setNodeId(1);
  node_shard_list.setLeaderShardList({0, 1, 2, 3, 4});
  node_shard_list.setFollowerShardList({5, 6});
  node_shard_list.setIsEdgeNode(false);

  // 初始化 状态
  auto table = std::make_shared<laser::TableSchema>();
  table->setDatabaseName(database_name_);
  table->setTableName(table_name_);
  table->setPartitionNumber(10);
  std::unordered_map<uint64_t, std::shared_ptr<laser::TableSchema>> tables({{1, table}});

  EXPECT_CALL(*config_, getShardNumber()).Times(10).WillRepeatedly(::testing::Return(folly::Optional<uint32_t>(10)));

  partition_manager_->subscribe([this](const laser::PartitionPtrSet& mount_partitions,
                                       const laser::PartitionPtrSet& unmount_partitions) {
    laser::PartitionPtrSet ex_mount_partitions({std::make_shared<laser::Partition>(database_name_, table_name_, 0),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 6),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 3),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 5),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 2),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 7),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 8),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 9)});
    expectPartitionPtrSet(ex_mount_partitions, mount_partitions);
    EXPECT_THAT(std::vector<uint32_t>({0, 1, 2, 3, 4}),
                ::testing::ContainerEq(partition_manager_->getLeaderShardList()));
    EXPECT_THAT(std::vector<uint32_t>({5, 6}), ::testing::ContainerEq(partition_manager_->getFollowerShardList()));
  });
  config_->update(node_shard_list, tables);

  // 增加和减少该 节点上的分片
  node_shard_list.setFollowerShardList({6, 7, 8, 9});
  EXPECT_CALL(*config_, getShardNumber()).Times(10).WillRepeatedly(::testing::Return(folly::Optional<uint32_t>(10)));

  partition_manager_->subscribe([this](const laser::PartitionPtrSet& mount_partitions,
                                       const laser::PartitionPtrSet& unmount_partitions) {
    laser::PartitionPtrSet ex_unmount_partitions({std::make_shared<laser::Partition>(database_name_, table_name_, 0),
                                                  std::make_shared<laser::Partition>(database_name_, table_name_, 6)});
    expectPartitionPtrSet(ex_unmount_partitions, unmount_partitions);

    laser::PartitionPtrSet ex_mount_partitions({std::make_shared<laser::Partition>(database_name_, table_name_, 1),
                                                std::make_shared<laser::Partition>(database_name_, table_name_, 4)});
    expectPartitionPtrSet(ex_mount_partitions, mount_partitions);
    EXPECT_THAT(std::vector<uint32_t>({6, 7, 8, 9}),
                ::testing::ContainerEq(partition_manager_->getFollowerShardList()));
  });
  config_->update(node_shard_list, tables);

  // 角色切换
  node_shard_list.setLeaderShardList({0, 1, 3, 4, 6});
  node_shard_list.setFollowerShardList({2, 7, 8, 9});
  EXPECT_CALL(*config_, getShardNumber()).Times(10).WillRepeatedly(::testing::Return(folly::Optional<uint32_t>(10)));

  partition_manager_->subscribe([this](const laser::PartitionPtrSet& mount_partitions,
                                       const laser::PartitionPtrSet& unmount_partitions) {
    laser::PartitionPtrSet ex_mount_partitions({std::make_shared<laser::Partition>(database_name_, table_name_, 3)});
    expectPartitionPtrSet(ex_mount_partitions, mount_partitions);
    EXPECT_THAT(std::vector<uint32_t>({0, 1, 3, 4, 6}),
                ::testing::ContainerEq(partition_manager_->getLeaderShardList()));
    EXPECT_THAT(std::vector<uint32_t>({2, 7, 8, 9}),
                ::testing::ContainerEq(partition_manager_->getFollowerShardList()));
  });
  config_->update(node_shard_list, tables);
}
