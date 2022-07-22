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
 * @author liubang <it.liubang@gmail.com>
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "common/hdfs/hdfsmonitor.h"
#include "laser/server/database_manager.h"
#include "laser/server/partition_handler.h"

class MockConfigManager : public laser::ConfigManager {
 public:
  explicit MockConfigManager(const std::shared_ptr<service_router::Router> router) : laser::ConfigManager(router) {}
};

class MockRouter : public service_router::Router {
 public:
  MockRouter() : service_router::Router() {}
  MOCK_METHOD2(setFollowerAvailableShardList, void(const service_router::Server&, const std::vector<uint32_t>&));
  MOCK_METHOD2(setAvailableShardList, void(const service_router::Server&, const std::vector<uint32_t>&));
};

class MockPartitionManager : public laser::PartitionManager {
 public:
  explicit MockPartitionManager(std::shared_ptr<laser::ConfigManager> config)
      : laser::PartitionManager(config, "test", 1, "default") {}
  void subscribe(laser::NotifyPartitionUpdate notify) override { update_callback_ = std::move(notify); }
  MOCK_METHOD0(getLeaderShardList, const std::vector<uint32_t>());
  MOCK_METHOD0(getFollowerShardList, const std::vector<uint32_t>());

 private:
  laser::NotifyPartitionUpdate update_callback_;
};

class MockPartitionHandler : public laser::PartitionHandler {
 public:
  explicit MockPartitionHandler(std::shared_ptr<laser::Partition> part)
      : laser::PartitionHandler(part, nullptr, nullptr, nullptr, nullptr, nullptr, 0) {}
  MOCK_METHOD0(getPartition, const std::shared_ptr<laser::Partition>());
  MOCK_METHOD1(setPartition, void(std::shared_ptr<laser::Partition>));
  MOCK_METHOD0(init, bool());
};

class MockReplicatorManager : public laser::ReplicatorManager {
 public:
  MockReplicatorManager() : laser::ReplicatorManager() {}
  MOCK_METHOD5(init, void(const std::string&, const std::string&, uint32_t, int64_t, const std::string&));
  MOCK_METHOD2(setShardList, void(const std::vector<uint32_t>&, const std::vector<uint32_t>&));
};

class MockDatabaseMetaInfo : public laser::DatabaseMetaInfo {
 public:
  MockDatabaseMetaInfo() : laser::DatabaseMetaInfo() {}
  MOCK_METHOD1(init, bool(const rocksdb::Options&));
};

class MockRocksDbConfigFactory : public laser::RocksDbConfigFactory {
 public:
  MockRocksDbConfigFactory() : laser::RocksDbConfigFactory(nullptr) {}
  void init(laser::RocksDbConfigUpdateCallback callback, folly::EventBase* evb) { callback(); }
  MOCK_CONST_METHOD0(getDefaultOptions, const folly::Optional<rocksdb::Options>());
};

class MockTableMonitor : public laser::TableMonitor {
 public:
  explicit MockTableMonitor(std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor)
      : laser::TableMonitor("test", "user_info", hdfs_monitor) {}
  MOCK_METHOD1(updateBase, void(const std::string& version));
  MOCK_METHOD1(addPartition, void(uint32_t partition_id));
  MOCK_METHOD1(removePartition, void(uint32_t partition_id));

  void subscribeBaseLoad(laser::NotifyPartitionLoadBaseData load_callback) {
    load_callback_ = std::move(load_callback);
  }

 private:
  laser::NotifyPartitionLoadBaseData load_callback_;
};

class MockDatabaseManager : public laser::DatabaseManager {
 public:
  MockDatabaseManager(std::shared_ptr<MockConfigManager> config, const std::string& group_name, uint32_t node_id)
      : laser::DatabaseManager(config, group_name, node_id, "default") {
    config_ = config;
  }
  MOCK_METHOD0(createPartitionManager, std::shared_ptr<laser::PartitionManager>());
  MOCK_METHOD1(getOrCreateTableMonitor, std::shared_ptr<laser::TableMonitor>(const std::shared_ptr<laser::Partition>&));
  MOCK_METHOD2(getOrCreateTableMonitor, std::shared_ptr<laser::TableMonitor>(const std::string&, const std::string&));
  MOCK_METHOD0(createReplicatorManager, std::shared_ptr<laser::ReplicatorManager>());
  MOCK_METHOD0(createRocksDbConfigFactory, std::shared_ptr<laser::RocksDbConfigFactory>());
  MOCK_METHOD1(getOrCreatePartitionHandler,
               std::shared_ptr<laser::PartitionHandler>(const std::shared_ptr<laser::Partition>&));
  MOCK_METHOD0(createDatabaseMetaInfo, std::shared_ptr<laser::DatabaseMetaInfo>());

 private:
  std::shared_ptr<MockConfigManager> config_;
};

class DatabaseManagerTest : public ::testing::Test {
 public:
  DatabaseManagerTest() {
    // router 中 metrics 依赖单例
    folly::SingletonVault::singleton()->registrationComplete();
    router_ = std::make_shared<MockRouter>();
    config_ = std::make_shared<MockConfigManager>(router_);
    hdfs_monitor_manager_ = std::make_shared<hdfs::HdfsMonitorManager>(1);
  }
  virtual ~DatabaseManagerTest() = default;
  virtual void TearDown() {}
  virtual void SetUp() { database_manager_ = std::make_shared<MockDatabaseManager>(config_, "test_group", 1); }

  void initTest() {
    rocksdb_config_factory_ = std::make_shared<MockRocksDbConfigFactory>();
    EXPECT_CALL(*rocksdb_config_factory_, getDefaultOptions()).WillRepeatedly(::testing::Return(rocksdb::Options()));
    EXPECT_CALL(*database_manager_, createRocksDbConfigFactory())
        .Times(1)
        .WillOnce(::testing::Return(rocksdb_config_factory_));

    partition_manager_ = std::make_shared<MockPartitionManager>(config_);
    auto replicator_manager_ = std::make_shared<MockReplicatorManager>();
    EXPECT_CALL(*database_manager_, createPartitionManager()).Times(1).WillOnce(::testing::Return(partition_manager_));
    EXPECT_CALL(*database_manager_, createReplicatorManager())
        .Times(1)
        .WillOnce(::testing::Return(replicator_manager_));

    std::string replicator_name = "replicator";
    std::string host_name = "127.0.0.1";
    uint32_t port = 1111;
    EXPECT_CALL(*replicator_manager_,
                init(::testing::Eq(replicator_name), ::testing::Eq(host_name), ::testing::Eq(port), ::testing::_, ::testing::_))
        .Times(1);
    database_manager_->init(1, replicator_name, host_name, port);
  }

 protected:
  std::shared_ptr<MockRocksDbConfigFactory> rocksdb_config_factory_;
  std::shared_ptr<MockDatabaseManager> database_manager_;
  std::shared_ptr<MockPartitionManager> partition_manager_;
  std::shared_ptr<MockReplicatorManager> replicator_manager_;
  std::shared_ptr<MockRouter> router_;
  std::shared_ptr<MockConfigManager> config_;
  std::shared_ptr<hdfs::HdfsMonitorManager> hdfs_monitor_manager_;
};

TEST_F(DatabaseManagerTest, syncDatabaseManagerAndThriftServer) {
  initTest();
  int update_shard_count = 0;
  std::thread thrift_server_thread([this, &update_shard_count]() {
    service_router::Server server;
    database_manager_->setServiceServer(server);
    update_shard_count++;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(0, update_shard_count);

  EXPECT_CALL(*partition_manager_, getLeaderShardList())
      .Times(3)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>()));
  EXPECT_CALL(*partition_manager_, getFollowerShardList())
      .Times(3)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>()));
  EXPECT_CALL(*router_, setFollowerAvailableShardList(::testing::_, ::testing::ContainerEq(std::vector<uint32_t>())))
      .Times(1);
  EXPECT_CALL(*router_, setAvailableShardList(::testing::_, ::testing::ContainerEq(std::vector<uint32_t>()))).Times(1);

  auto meta_info = std::make_shared<MockDatabaseMetaInfo>();
  EXPECT_CALL(*database_manager_, createDatabaseMetaInfo()).Times(1).WillOnce(::testing::Return(meta_info));
  EXPECT_CALL(*meta_info, init(::testing::_)).Times(1).WillOnce(::testing::Return(true));

  laser::PartitionPtrSet mount_partitions;
  laser::PartitionPtrSet unmount_partitions;
  database_manager_->updatePartitions(mount_partitions, unmount_partitions);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(1, update_shard_count);
  thrift_server_thread.join();
}

TEST_F(DatabaseManagerTest, syncDatabaseManagerAndThriftServerRevert) {
  initTest();

  // 首先在 thrift server 没有初始化好调用 updatePartitions 模拟配置更新先调用
  // 后等server 启动后再调用一次 updatePartitions
  //
  // 对于 ReplicatorManager 的 shard 每次都更新
  // 对于 thrift server 的仅调用一次
  // 另外在 setServiceServer 后也会调用一次
  int update_shard_count = 0;
  std::thread thrift_server_thread([this, &update_shard_count]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    service_router::Server server;
    database_manager_->setServiceServer(server);
    update_shard_count++;
  });
  EXPECT_EQ(0, update_shard_count);

  EXPECT_CALL(*partition_manager_, getLeaderShardList())
      .Times(5)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>({0, 1, 2, 3, 4})));
  EXPECT_CALL(*partition_manager_, getFollowerShardList())
      .Times(5)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>()));

  EXPECT_CALL(*router_,
              setAvailableShardList(::testing::_, ::testing::ContainerEq(std::vector<uint32_t>({0, 1, 2, 3, 4}))))
      .Times(2);
  EXPECT_CALL(*router_, setFollowerAvailableShardList(::testing::_, ::testing::ContainerEq(std::vector<uint32_t>())))
      .Times(2);

  auto meta_info = std::make_shared<MockDatabaseMetaInfo>();
  EXPECT_CALL(*database_manager_, createDatabaseMetaInfo()).Times(1).WillOnce(::testing::Return(meta_info));
  EXPECT_CALL(*meta_info, init(::testing::_)).Times(1).WillOnce(::testing::Return(true));

  laser::PartitionPtrSet mount_partitions;
  laser::PartitionPtrSet unmount_partitions;
  database_manager_->updatePartitions(mount_partitions, unmount_partitions);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_EQ(1, update_shard_count);

  // update shard list
  database_manager_->updatePartitions(mount_partitions, unmount_partitions);

  thrift_server_thread.join();
}

TEST_F(DatabaseManagerTest, monitor) {
  initTest();
  std::string database_name = "test";
  std::string table_name = "user_info";
  auto monitor = std::make_shared<MockTableMonitor>(hdfs_monitor_manager_);
  EXPECT_CALL(*database_manager_, getOrCreateTableMonitor(::testing::_))
      .Times(3)
      .WillRepeatedly(::testing::Return(monitor));

  EXPECT_CALL(*monitor, addPartition(::testing::Eq(0))).Times(1);
  EXPECT_CALL(*monitor, addPartition(::testing::Eq(2))).Times(1);
  EXPECT_CALL(*monitor, removePartition(::testing::Eq(1))).Times(1);

  auto part0 = std::make_shared<laser::Partition>(database_name, table_name, 0);
  auto part1 = std::make_shared<laser::Partition>(database_name, table_name, 1);
  auto part2 = std::make_shared<laser::Partition>(database_name, table_name, 2);

  auto partition_handler = std::make_shared<MockPartitionHandler>(part0);
  EXPECT_CALL(*database_manager_, getOrCreatePartitionHandler(::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::Return(partition_handler));
  EXPECT_CALL(*partition_handler, getPartition())
      .Times(2)
      .WillOnce(::testing::Return(part0))
      .WillOnce(::testing::Return(part2));

  EXPECT_CALL(*partition_manager_, getLeaderShardList())
      .Times(1)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>({0, 1, 2, 3, 4})));
  EXPECT_CALL(*partition_manager_, getFollowerShardList())
      .Times(1)
      .WillRepeatedly(::testing::Return(std::vector<uint32_t>()));

  auto meta_info = std::make_shared<MockDatabaseMetaInfo>();
  EXPECT_CALL(*database_manager_, createDatabaseMetaInfo()).Times(1).WillOnce(::testing::Return(meta_info));
  EXPECT_CALL(*meta_info, init(::testing::_)).Times(1).WillOnce(::testing::Return(true));

  laser::PartitionPtrSet mount_partitions({part0, part2});
  laser::PartitionPtrSet unmount_partitions({part1});
  database_manager_->updatePartitions(mount_partitions, unmount_partitions);
}

TEST_F(DatabaseManagerTest, trigger) {
  initTest();
  std::string database_name = "test";
  std::string table_name = "user_info";
  auto monitor = std::make_shared<MockTableMonitor>(hdfs_monitor_manager_);
  EXPECT_CALL(*database_manager_, getOrCreateTableMonitor(::testing::Eq(database_name), ::testing::Eq(table_name)))
      .Times(1)
      .WillRepeatedly(::testing::Return(monitor));

  std::string version = "test_version";
  EXPECT_CALL(*monitor, updateBase(::testing::Eq(version))).Times(1);

  database_manager_->triggerBase(database_name, table_name, version);
}
