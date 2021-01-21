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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "boost/filesystem.hpp"

#include "laser/server/partition_handler.h"
#include "laser/server/database_manager.h"

class MockDatabaseManager : public laser::DatabaseManager {
 public:
  MockDatabaseManager(const std::string& group_name, uint32_t node_id)
      : laser::DatabaseManager(nullptr, group_name, node_id) {}
  MOCK_METHOD0(createRocksDbConfigFactory, std::shared_ptr<laser::RocksDbConfigFactory>());
  MOCK_METHOD0(getRocksdbConfigFactory, std::shared_ptr<laser::RocksDbConfigFactory>());
};

class MockRocksDbConfigFactory : public laser::RocksDbConfigFactory {
 public:
  MockRocksDbConfigFactory() : laser::RocksDbConfigFactory(nullptr) {}
  void init(laser::RocksDbConfigUpdateCallback callback, folly::EventBase* evb) { callback(); }
  MOCK_CONST_METHOD0(getDefaultOptions, const folly::Optional<rocksdb::Options>());
  MOCK_CONST_METHOD2(getVersionedOptions,
                     const folly::Optional<laser::VersionedOptions>(const std::string&, const std::string&));
  MOCK_CONST_METHOD3(hasOptionsChanged, bool(const std::string&, const std::string&, const laser::VersionedOptions&));
};

class MockReplicationDB : public laser::ReplicationDB {
 public:
  explicit MockReplicationDB(const rocksdb::Options& options) : laser::ReplicationDB("", options) {}
  void setUpdateVersionCallback(laser::UpdateVersionCallback callback) override {
    update_version_callback_ = std::move(callback);
  }

  void triggerUpdate(uint64_t db_hash, const std::string& version) { update_version_callback_(db_hash, version); }

  uint64_t getProperty(const std::string& key) override { return 0; }
 private:
  laser::UpdateVersionCallback update_version_callback_;
};

class MockRocksDbEngine : public laser::RocksDbEngine {
 public:
  MockRocksDbEngine() : laser::RocksDbEngine(nullptr) {
    rocksdb::Options options;
    replicator_db_ = std::make_shared<MockReplicationDB>(options);
  }
  MOCK_METHOD1(ingestBaseSst, laser::Status(const std::string&));
  MOCK_METHOD2(ingestDeltaSst, laser::Status(const std::string&, const std::string&));

  std::weak_ptr<laser::ReplicationDB> getReplicationDB() override { return replicator_db_; }

  std::weak_ptr<MockReplicationDB> getMockReplicationDB() { return replicator_db_; }

 private:
  std::shared_ptr<MockReplicationDB> replicator_db_;
};

class MockReplicatorManager : public laser::ReplicatorManager {
 public:
  MockReplicatorManager() : laser::ReplicatorManager() {}
  MOCK_METHOD6(addDB, void(const laser::DBRole&, uint32_t, int64_t, const std::string&,
                           std::shared_ptr<laser::WdtReplicatorManager>, std::weak_ptr<laser::ReplicationDB>));
};

class MockDatabaseMetaInfo : public laser::DatabaseMetaInfo {
 public:
  MockDatabaseMetaInfo() : laser::DatabaseMetaInfo() {}
  MOCK_METHOD1(getVersion, const std::string(std::shared_ptr<laser::Partition>));
  MOCK_METHOD2(updateVersion, void(std::shared_ptr<laser::Partition>, const std::string&));
  MOCK_METHOD1(getDeltaVersions, const std::vector<std::string>(std::shared_ptr<laser::Partition>));
  MOCK_METHOD2(updateDeltaVersions, void(std::shared_ptr<laser::Partition>, const std::vector<std::string>&));
};

class MockPartitionHandler : public laser::PartitionHandler {
 public:
  MockPartitionHandler(std::shared_ptr<laser::Partition> partition, laser::DatabaseManager* database_manager,
                       laser::DatabaseMetaInfo* database_meta_info, laser::ReplicatorManager* replicator_manager,
                       std::shared_ptr<laser::WdtReplicatorManager> wdt_manager, folly::EventBase* evb)
      : laser::PartitionHandler(partition, database_manager, database_meta_info, replicator_manager, wdt_manager, evb,
                                0) {}
  MOCK_METHOD0(createLoaderQueue, std::shared_ptr<folly::ProducerConsumerQueue<laser::PartitionLoadInfo>>());
  MOCK_METHOD2(createRocksDbEngine, bool(std::shared_ptr<laser::RocksDbEngine>*, const std::string&));
  MOCK_METHOD2(baseDataReplicate, void(const std::string&, const std::string&));

  bool ingestBaseData(std::shared_ptr<laser::RocksDbEngine>* db, const std::string& version) override {
    return laser::PartitionHandler::ingestBaseData(db, version);
  }

  bool ingestDeltaData(const std::string& version) override {
    return laser::PartitionHandler::ingestDeltaData(version);
  }

  void setUpdateVersionCallback(std::shared_ptr<laser::ReplicationDB> replication_db) {
    return laser::PartitionHandler::setUpdateVersionCallback(replication_db);
  }
};

namespace laser {
DECLARE_string(data_dir);
}

class PartitionHandlerTest : public ::testing::Test {
 public:
  PartitionHandlerTest() {
    folly::SingletonVault::singleton()->registrationComplete();
    partition_ = std::make_shared<laser::Partition>(database_name_, table_name_, part_id_);
    database_manager_ = std::make_shared<MockDatabaseManager>(group_name_, node_id_);
    rocksdb_config_factory_ = std::make_shared<MockRocksDbConfigFactory>();
    database_meta_info_ = std::make_shared<MockDatabaseMetaInfo>();
    replicator_manager_ = std::make_shared<MockReplicatorManager>();
    rocksdb_ = std::make_shared<MockRocksDbEngine>();
    wdt_manager_ = std::make_shared<laser::WdtReplicatorManager>("test", 1);
    queue_ = std::make_shared<folly::ProducerConsumerQueue<laser::PartitionLoadInfo>>(queue_size_);
    laser::FLAGS_data_dir = data_dir_;
  }
  virtual ~PartitionHandlerTest() = default;
  virtual void TearDown() {
    boost::filesystem::path bpath(data_dir_);
    if (boost::filesystem::exists(data_dir_)) {
      boost::filesystem::remove_all(data_dir_);
    }
  }
  virtual void SetUp() {
    partition_handler_ =
        std::make_shared<MockPartitionHandler>(partition_, database_manager_.get(), database_meta_info_.get(),
                                               replicator_manager_.get(), wdt_manager_, folly::getEventBase());
    initTest();
  }

 protected:
  std::shared_ptr<MockDatabaseManager> database_manager_;
  std::shared_ptr<MockRocksDbConfigFactory> rocksdb_config_factory_;
  std::shared_ptr<MockDatabaseMetaInfo> database_meta_info_;
  std::shared_ptr<MockReplicatorManager> replicator_manager_;
  std::shared_ptr<laser::Partition> partition_;
  std::shared_ptr<MockRocksDbEngine> rocksdb_;
  std::shared_ptr<MockPartitionHandler> partition_handler_;
  std::shared_ptr<laser::WdtReplicatorManager> wdt_manager_;
  std::shared_ptr<folly::ProducerConsumerQueue<laser::PartitionLoadInfo>> queue_;
  std::string database_name_ = "test";
  std::string table_name_ = "user_info";
  uint32_t part_id_ = 1;
  std::string group_name_ = "group_test";
  uint32_t node_id_ = 1;
  uint32_t queue_size_ = 2;
  std::string init_base_version_ = "20190216180723_5376625654628601422";
  std::string data_dir_ = "/tmp/partition_handler_unit_test";

  void initTest() {
    EXPECT_CALL(*database_manager_, createRocksDbConfigFactory())
        .WillRepeatedly(::testing::Return(rocksdb_config_factory_));
    EXPECT_CALL(*database_manager_, getRocksdbConfigFactory())
        .WillRepeatedly(::testing::Return(rocksdb_config_factory_));

    laser::VersionedOptions versioned_options =
        laser::VersionedOptions(std::make_shared<rocksdb::Options>(), "config1", 1);
    EXPECT_CALL(*rocksdb_config_factory_, getVersionedOptions(::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(versioned_options));
    EXPECT_CALL(*rocksdb_config_factory_, hasOptionsChanged(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(::testing::Return(true))
        .WillRepeatedly(::testing::Return(false));
    EXPECT_CALL(*partition_handler_, createLoaderQueue()).Times(1).WillOnce(::testing::Return(queue_));
    EXPECT_CALL(*database_meta_info_, getVersion(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(init_base_version_));
    EXPECT_CALL(*database_meta_info_, getDeltaVersions(::testing::_)).Times(1);
    EXPECT_CALL(*partition_handler_, createRocksDbEngine(::testing::_, ::testing::Eq(init_base_version_)))
        .Times(1)
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(rocksdb_), ::testing::Return(true)));
    EXPECT_CALL(*replicator_manager_, addDB(::testing::_, ::testing::_, ::testing::Eq(partition_->getPartitionHash()),
                                            ::testing::Eq(init_base_version_), ::testing::_, ::testing::_))
        .Times(1);
    EXPECT_CALL(*database_meta_info_, updateVersion(::testing::_, ::testing::Eq(init_base_version_))).Times(1);
    EXPECT_CALL(*database_meta_info_, updateDeltaVersions(::testing::_, ::testing::_)).Times(1);
    partition_handler_->init();

    laser::PartitionMetaInfo info;
    partition_handler_->getPartitionMetaInfo(&info);
    EXPECT_EQ(init_base_version_, info.getBaseVersion());
  }

  void ingestBaseDataMock(const std::string& version, uint32_t sleep_ms) {
    std::string base_data_file = laser::DataPathManager::getSourceBaseDataFile(partition_, version);
    boost::filesystem::path base_data_file_path(base_data_file);
    boost::filesystem::create_directories(base_data_file_path.parent_path());
    folly::writeFile(/* write random data */ version, base_data_file.c_str());

    EXPECT_CALL(*partition_handler_, createRocksDbEngine(::testing::_, ::testing::Eq(version)))
        .Times(1)
        .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(rocksdb_), ::testing::Return(true)));

    EXPECT_CALL(*rocksdb_, ingestBaseSst(::testing::Eq(base_data_file)))
        .Times(1)
        .WillOnce(::testing::DoAll(::testing::InvokeWithoutArgs([sleep_ms]() {
                                     if (sleep_ms > 0) {
                                       std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                                     }
                                   }),
                                   ::testing::Return(laser::Status::OK)));
  }

  void ingestDeltaDataMock(const std::string& base_version, const std::string& version, uint32_t sleep_ms) {
    std::string delta_sst_path = laser::DataPathManager::getSourceDeltaDataFile(partition_, base_version, version);
    boost::filesystem::path delta_sst_path_path(delta_sst_path);
    boost::filesystem::create_directories(delta_sst_path_path.parent_path());
    folly::writeFile(/* write random data */ version, delta_sst_path.c_str());

    EXPECT_CALL(*rocksdb_, ingestDeltaSst(::testing::Eq(delta_sst_path), ::testing::_))
        .Times(1)
        .WillOnce(::testing::DoAll(::testing::InvokeWithoutArgs([sleep_ms]() {
                                     if (sleep_ms > 0) {
                                       std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                                     }
                                   }),
                                   ::testing::Return(laser::Status::OK)));
  }
};

TEST_F(PartitionHandlerTest, ingestBaseData) {
  std::shared_ptr<laser::RocksDbEngine> db;
  // base data not exists
  EXPECT_FALSE(partition_handler_->ingestBaseData(&db, init_base_version_));

  // current data dir has exists and create fail
  EXPECT_CALL(*partition_handler_, createRocksDbEngine(::testing::_, ::testing::Eq(init_base_version_)))
      .Times(1)
      .WillOnce(::testing::Return(false));
  std::string base_data_file = laser::DataPathManager::getSourceBaseDataFile(partition_, init_base_version_);
  boost::filesystem::path base_data_file_path(base_data_file);
  boost::filesystem::create_directories(base_data_file_path.parent_path());
  folly::writeFile(/* write random data */ init_base_version_, base_data_file.c_str());
  std::string current_data_dir =
      laser::DataPathManager::getDatabaseDataDir(database_manager_.get(), partition_, init_base_version_);
  boost::filesystem::path current_data_dir_path(current_data_dir);
  boost::filesystem::create_directories(current_data_dir_path);
  EXPECT_FALSE(partition_handler_->ingestBaseData(&db, init_base_version_));
}

TEST_F(PartitionHandlerTest, ingestBaseDataForIngest) {
  std::shared_ptr<laser::RocksDbEngine> db;
  std::string base_data_file = laser::DataPathManager::getSourceBaseDataFile(partition_, init_base_version_);
  boost::filesystem::path base_data_file_path(base_data_file);
  boost::filesystem::create_directories(base_data_file_path.parent_path());
  folly::writeFile(/* write random data */ init_base_version_, base_data_file.c_str());

  EXPECT_CALL(*partition_handler_, createRocksDbEngine(::testing::_, ::testing::Eq(init_base_version_)))
      .Times(2)
      .WillRepeatedly(::testing::DoAll(::testing::SetArgPointee<0>(rocksdb_), ::testing::Return(true)));

  EXPECT_CALL(*rocksdb_, ingestBaseSst(::testing::Eq(base_data_file)))
      .Times(1)
      .WillOnce(::testing::Return(laser::Status::RS_IO_ERROR));
  EXPECT_FALSE(partition_handler_->ingestBaseData(&db, init_base_version_));

  EXPECT_CALL(*rocksdb_, ingestBaseSst(::testing::Eq(base_data_file)))
      .Times(1)
      .WillOnce(::testing::Return(laser::Status::OK));
  EXPECT_TRUE(partition_handler_->ingestBaseData(&db, init_base_version_));
}

TEST_F(PartitionHandlerTest, ingestDeltaData) {
  std::string delta_version = "delta_version_foo";
  // base data not exists
  EXPECT_FALSE(partition_handler_->ingestDeltaData(delta_version));

  // current data dir has exists and create fail
  std::string delta_sst_path =
      laser::DataPathManager::getSourceDeltaDataFile(partition_, init_base_version_, delta_version);
  boost::filesystem::path delta_sst_path_path(delta_sst_path);
  boost::filesystem::create_directories(delta_sst_path_path.parent_path());
  folly::writeFile(/* write random data */ delta_version, delta_sst_path.c_str());

  EXPECT_CALL(*rocksdb_, ingestDeltaSst(::testing::Eq(delta_sst_path), ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(laser::Status::RS_IO_ERROR));
  EXPECT_FALSE(partition_handler_->ingestDeltaData(delta_version));

  EXPECT_CALL(*rocksdb_, ingestDeltaSst(::testing::Eq(delta_sst_path), ::testing::_))
      .Times(1)
      .WillOnce(::testing::Return(laser::Status::OK));
  EXPECT_TRUE(partition_handler_->ingestDeltaData(delta_version));
}

// 相同的 version 不执行 base load
TEST_F(PartitionHandlerTest, loadBaseDataVersionInvalid) { partition_handler_->loadBaseData(init_base_version_); }

// 正常执行
TEST_F(PartitionHandlerTest, loadBaseDataSuccess) {
  std::string load_base_version = "20190216180723_5376625654628601423";

  ingestBaseDataMock(load_base_version, 0);
  EXPECT_CALL(*replicator_manager_, addDB(::testing::_, ::testing::_, ::testing::Eq(partition_->getPartitionHash()),
                                          ::testing::Eq(load_base_version), ::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*database_meta_info_, updateVersion(::testing::_, ::testing::Eq(load_base_version))).Times(1);
  EXPECT_CALL(*database_meta_info_, updateDeltaVersions(::testing::_, ::testing::_)).Times(1);

  partition_handler_->loadBaseData(load_base_version);

  laser::PartitionMetaInfo info;
  partition_handler_->getPartitionMetaInfo(&info);
  EXPECT_EQ(load_base_version, info.getBaseVersion());
}

// 测试同时 load 多个版本
TEST_F(PartitionHandlerTest, loadBaseDataMultiVersion) {
  std::string load_base_version_1 = "20190216180723_5376625654628601423";
  std::string load_base_version_2 = "20190216180723_5376625654628601424";
  std::string load_base_version_3 = "20190216180723_5376625654628601425";

  // 队列目前最大长度是3，所有如果同时操作4个以上就会直接扔掉，对于真实代码默认是10，
  // 出现并发 load base 的情况只有通过 http 接口频繁调用导致，系统自动调度 hdfs load base 一般情况下不会触发
  EXPECT_CALL(*replicator_manager_, addDB(::testing::_, ::testing::_, ::testing::Eq(partition_->getPartitionHash()),
                                          ::testing::Eq(load_base_version_1), ::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*database_meta_info_, updateVersion(::testing::_, ::testing::Eq(load_base_version_1))).Times(1);
  EXPECT_CALL(*database_meta_info_, updateDeltaVersions(::testing::_, ::testing::_)).Times(2);
  EXPECT_CALL(*replicator_manager_, addDB(::testing::_, ::testing::_, ::testing::Eq(partition_->getPartitionHash()),
                                          ::testing::Eq(load_base_version_2), ::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*database_meta_info_, updateVersion(::testing::_, ::testing::Eq(load_base_version_2))).Times(1);
  ingestBaseDataMock(load_base_version_1, 30);
  ingestBaseDataMock(load_base_version_2, 10);

  std::thread first_load_base([this, load_base_version_1]() {
    partition_handler_->loadBaseData(load_base_version_1);
    EXPECT_EQ(0, queue_->sizeGuess());
  });
  auto cpu_pool = std::make_shared<folly::CPUThreadPoolExecutor>(10);
  cpu_pool->add([this, load_base_version_2]() {
    partition_handler_->loadBaseData(load_base_version_2);
    EXPECT_EQ(1, queue_->sizeGuess());
  });
  // 确保后面添加加载的顺序
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  cpu_pool->add([this, load_base_version_3]() {
    // 本次更新version 会抛弃
    partition_handler_->loadBaseData(load_base_version_3);
    EXPECT_EQ(1, queue_->sizeGuess());
  });
  first_load_base.join();

  laser::PartitionMetaInfo info;
  partition_handler_->getPartitionMetaInfo(&info);
  EXPECT_EQ(load_base_version_2, info.getBaseVersion());
}

// 不同的 base version 不执行 delta load
TEST_F(PartitionHandlerTest, loadDeltaDataVersionInvalid) {
  std::string base_version = "20190216180723_5376625654628601423";
  std::vector<std::string> delta_versions({"20190216180723_5376625654628601424"});
  partition_handler_->loadDeltaData(base_version, delta_versions);
}

// 正常执行
TEST_F(PartitionHandlerTest, loadDeltaDataSuccess) {
  std::string delta_version_1 = "20190216180723_5376625654628601423";
  std::string delta_version_2 = "20190216180723_5376625654628601424";
  std::string delta_version_3 = "20190216180723_5376625654628601424";  // version 相同, 不重复加载
  std::vector<std::string> delta_versions({delta_version_1, delta_version_2, delta_version_3});

  ingestDeltaDataMock(init_base_version_, delta_version_1, 0);
  ingestDeltaDataMock(init_base_version_, delta_version_2, 0);

  EXPECT_CALL(*database_meta_info_, updateDeltaVersions(::testing::_, ::testing::_)).Times(1);
  partition_handler_->loadDeltaData(init_base_version_, delta_versions);

  laser::PartitionMetaInfo info;
  partition_handler_->getPartitionMetaInfo(&info);
  EXPECT_EQ(init_base_version_, info.getBaseVersion());
  EXPECT_THAT(std::vector<std::string>({delta_version_1, delta_version_2}),
              ::testing::ContainerEq(info.getDeltaVersions()));
}

// 测试同时 load base delta 情况
TEST_F(PartitionHandlerTest, loadBaseAndDeltaDataMultiVersion) {
  std::string load_base_version_1 = "20190216180723_5376625654628601423";
  std::string load_base_version_2 = "20190216180723_5376625654628601424";

  std::string delta_version_1 = "20190216180723_5376625654628601423";
  std::string delta_version_2 = "20190216180723_5376625654628601424";
  std::vector<std::string> delta_versions({delta_version_1, delta_version_2});

  // 先加载 base version1 --> delta -> base version2
  // 测试结果： base version2 将丢弃
  EXPECT_CALL(*replicator_manager_, addDB(::testing::_, ::testing::_, ::testing::Eq(partition_->getPartitionHash()),
                                          ::testing::Eq(load_base_version_1), ::testing::_, ::testing::_))
      .Times(1);
  EXPECT_CALL(*database_meta_info_, updateVersion(::testing::_, ::testing::Eq(load_base_version_1))).Times(1);
  EXPECT_CALL(*database_meta_info_, updateDeltaVersions(::testing::_, ::testing::_)).Times(2);
  ingestBaseDataMock(load_base_version_1, 30);
  ingestDeltaDataMock(load_base_version_1, delta_version_1, 10);
  ingestDeltaDataMock(load_base_version_1, delta_version_2, 10);

  std::thread first_load_base([this, load_base_version_1]() {
    partition_handler_->loadBaseData(load_base_version_1);
    EXPECT_EQ(0, queue_->sizeGuess());
  });
  auto cpu_pool = std::make_shared<folly::CPUThreadPoolExecutor>(10);
  cpu_pool->add([this, load_base_version_1, delta_versions]() {
    partition_handler_->loadDeltaData(load_base_version_1, delta_versions);
    EXPECT_EQ(1, queue_->sizeGuess());
  });
  // 确保后面添加加载的顺序
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  cpu_pool->add([this, load_base_version_2]() {
    // 本次更新version 会抛弃
    partition_handler_->loadBaseData(load_base_version_2);
    EXPECT_EQ(1, queue_->sizeGuess());
  });
  first_load_base.join();

  laser::PartitionMetaInfo info;
  partition_handler_->getPartitionMetaInfo(&info);
  EXPECT_EQ(load_base_version_1, info.getBaseVersion());
  EXPECT_THAT(delta_versions, ::testing::ContainerEq(info.getDeltaVersions()));
}

TEST_F(PartitionHandlerTest, baseVersionUpdate) {
  std::string new_version = "20190216180723_5376625654628601423";
  std::weak_ptr<MockReplicationDB> replicator_db = rocksdb_->getMockReplicationDB();
  partition_handler_->setUpdateVersionCallback(replicator_db.lock());
  EXPECT_CALL(*partition_handler_, baseDataReplicate(::testing::Eq("2222"), ::testing::Eq(new_version))).Times(1);

  // 只有 follower 角色才会执行更新版本回调操作
  partition_->setRole(laser::DBRole::FOLLOWER);
  replicator_db.lock()->triggerUpdate(2222, new_version);
}
