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
#include "boost/filesystem.hpp"

#include "folly/Singleton.h"
#include "folly/Random.h"
#include "laser/server/engine/rocksdb.h"

DECLARE_string(vmodule);

class MockReplicationDB : public laser::ReplicationDB {
 public:
  explicit MockReplicationDB(const std::string& path, const rocksdb::Options& options)
      : laser::ReplicationDB(path, options) {}
  laser::Status writeWithSeqNumber(rocksdb::WriteBatch& write_batch, int64_t* seq_no, int64_t write_ms) override {
    return laser::ReplicationDB::writeWithSeqNumber(write_batch, seq_no, write_ms);
  }
  void delayPullFromUpstream() override { return laser::ReplicationDB::delayPullFromUpstream(); }

  void getPullRequest(laser::ReplicateRequest* req, const laser::ReplicateType& type, int64_t max_seq_no = 0) override {
    laser::ReplicationDB::getPullRequest(req, type, max_seq_no);
  }

  void applyUpdates(folly::Try<laser::ReplicateResponse>&& try_response) override {
    laser::ReplicationDB::applyUpdates(std::move(try_response));
  }
  void getUpdates(laser::ReplicateResponse* response, std::unique_ptr<::laser::ReplicateRequest> request) {
    laser::ReplicationDB::getUpdates(response, std::move(request));
  }

  MOCK_METHOD0(pullFromUpstream, void());
};

namespace laser {
DECLARE_int32(replicator_pull_delay_on_error_ms);
DECLARE_int32(replicator_max_updates_per_response);
}
class ReplicationDBTest : public ::testing::Test {
 public:
  ReplicationDBTest() {
    folly::SingletonVault::singleton()->registrationComplete();
    replicate_thread_pool_ = std::make_shared<folly::IOThreadPoolExecutor>(1);
    laser::FLAGS_replicator_pull_delay_on_error_ms = error_delay_ms_;
    FLAGS_vmodule = "replication_db=5";
    wdt_manager_ = std::make_shared<laser::WdtReplicatorManager>("test", 1);
  }
  virtual ~ReplicationDBTest() = default;
  virtual void TearDown() {
    boost::filesystem::path bpath(data_dir_);
    if (boost::filesystem::exists(data_dir_)) {
      boost::filesystem::remove_all(data_dir_);
    }
  }
  virtual void SetUp() {}

 protected:
  uint32_t error_delay_ms_{5};
  uint32_t max_updates_{50};
  int64_t node_hash_ = 1000;
  std::string version_ = "base_version1";
  std::string data_dir_ = "/tmp/replication_db_unit_test/";
  std::shared_ptr<folly::IOThreadPoolExecutor> replicate_thread_pool_;
  std::shared_ptr<laser::WdtReplicatorManager> wdt_manager_;

  std::shared_ptr<MockReplicationDB> createDb(const laser::DBRole& role = laser::DBRole::LEADER) {
    int64_t db_hash = 0;
    return createDb(&db_hash, role, 0);
  }

  std::shared_ptr<MockReplicationDB> createDb(int64_t* new_db_hash, const laser::DBRole& role, int64_t db_hash) {
    rocksdb::Options options;
    options.create_if_missing = true;
    if (db_hash == 0) {
      db_hash = folly::Random::secureRand64();
    }
    *new_db_hash = db_hash;

    // 为了方便测试
    int64_t node_hash = db_hash + node_hash_;
    std::string path = folly::to<std::string>(data_dir_, db_hash);
    auto db = std::make_shared<MockReplicationDB>(path, options);
    std::string client_address = folly::to<std::string>("127.0.0.1", folly::Random::secureRand32(1000, 9999));

    if (role == laser::DBRole::FOLLOWER) {
      EXPECT_CALL(*db, pullFromUpstream()).Times(1);
    }

    db->open();
    db->startReplicator(0, db_hash, "test_replicate", replicate_thread_pool_, role, version_, node_hash, client_address,
                        wdt_manager_);
    return db;
  }

  laser::LaserKeyFormat createLaserKey(const std::string& key) {
    std::vector<std::string> primary_keys({"pk"});
    std::vector<std::string> column_names({key});
    laser::LaserKeyFormat test_key(primary_keys, column_names);
    return test_key;
  }

  void writeBatchData(std::shared_ptr<MockReplicationDB> db, const std::string& data_prefix, uint32_t number) {
    for (uint32_t i = 0; i < number; i++) {
      laser::RocksDbBatch batch;
      std::string data = folly::to<std::string>(data_prefix, i);
      laser::LaserValueRawString value(data);
      batch.iput(createLaserKey(data), value);
      laser::Status status = db->write(batch);
      EXPECT_EQ(laser::Status::OK, status);
    }
    checkReadData(db, data_prefix, number);
  }

  void writeBatchDataEach(std::shared_ptr<MockReplicationDB> db, const std::string& data_prefix, uint32_t number,
                          uint32_t batch_size) {
    uint32_t batch_times = std::ceil(number / static_cast<float>(batch_size));
    uint32_t data_id = 0;
    for (uint32_t i = 0; i < batch_times; i++) {
      laser::RocksDbBatch batch;
      for (uint32_t j = 0; j < batch_size && data_id < number; j++) {
        data_id = i * batch_size + j;
        std::string data = folly::to<std::string>(data_prefix, data_id);
        laser::LaserValueRawString value(data);
        batch.iput(createLaserKey(data), value);
      }
      laser::Status status = db->write(batch);
      EXPECT_EQ(laser::Status::OK, status);
    }
    checkReadData(db, data_prefix, number);
  }

  void checkReadData(std::shared_ptr<MockReplicationDB> db, const std::string& data_prefix, uint32_t number) {
    for (uint32_t i = 0; i < number; i++) {
      std::string data = folly::to<std::string>(data_prefix, i);
      laser::LaserValueRawString value;
      laser::Status status = db->read(&value, createLaserKey(data));
      EXPECT_EQ(laser::Status::OK, status);
      value.decode();
      EXPECT_EQ(data, value.getValue());
    }
  }

  void replicateForward(int64_t* next_seq_no, std::shared_ptr<MockReplicationDB> leader_db,
                        std::shared_ptr<MockReplicationDB> follower_db) {
    EXPECT_CALL(*follower_db, pullFromUpstream()).Times(1);
    folly::Promise<laser::ReplicateResponse> res_promise;
    laser::ReplicateResponse response;
    laser::ReplicateRequest request;
    follower_db->getPullRequest(&request, laser::ReplicateType::FORWARD, 0);
    *next_seq_no = request.seq_no;

    leader_db->getUpdates(&response, std::make_unique<laser::ReplicateRequest>(request));
    res_promise.setValue(std::move(response));

    auto future = res_promise.getFuture();
    future.then([follower_db](folly::Try<laser::ReplicateResponse>&& t) {
                  follower_db->applyUpdates(std::move(t));}).get();
  }

  void checkSeqNo(std::shared_ptr<MockReplicationDB> leader_db, std::shared_ptr<MockReplicationDB> follower_db) {
    laser::ReplicationDbMetaInfo leader_info;
    laser::ReplicationDbMetaInfo follower_info;

    leader_db->getDbMetaInfo(&leader_info);
    follower_db->getDbMetaInfo(&follower_info);

    EXPECT_EQ(leader_info.getSeqNo(), follower_info.getSeqNo());
  }
};

TEST_F(ReplicationDBTest, notAllowFollowerWrite) {
  auto follower_db = createDb(laser::DBRole::FOLLOWER);
  laser::RocksDbBatch batch;
  std::string data = "test";
  laser::LaserValueRawString value(data);
  batch.iput(createLaserKey(data), value);
  laser::Status status = follower_db->write(batch);
  EXPECT_EQ(laser::Status::RS_WRITE_IN_FOLLOWER, status);
}

TEST_F(ReplicationDBTest, writeAndRead) {
  laser::RocksDbBatch batch;

  for (int i = 0; i < 10; i++) {
    std::string data = folly::to<std::string>("test", i);
    laser::LaserValueRawString value(data);
    batch.iput(createLaserKey(data), value);
  }
  auto db = createDb();
  laser::Status status = db->write(batch);
  EXPECT_EQ(laser::Status::OK, status);
  for (int i = 0; i < 10; i++) {
    std::string data = folly::to<std::string>("test", i);
    laser::LaserValueRawString value;
    status = db->read(&value, createLaserKey(data));
    EXPECT_EQ(laser::Status::OK, status);
    value.decode();
    EXPECT_EQ(data, value.getValue());
  }
}

TEST_F(ReplicationDBTest, writeWithSeqNumber) {
  auto db = createDb();
  for (int i = 0; i < 10; i++) {
    rocksdb::WriteBatch write_batch;
    std::string data = folly::to<std::string>("test", i);
    rocksdb::Slice slice_key(data.data(), data.length());
    rocksdb::Slice slice_value(data.data(), data.length());
    write_batch.Put(slice_key, slice_value);

    int64_t seq_no = 0;
    int64_t time_ms = 0;
    laser::Status status = db->writeWithSeqNumber(write_batch, &seq_no, time_ms);
    EXPECT_EQ(laser::Status::OK, status);
    EXPECT_EQ(i + 1, seq_no);
  }
}

TEST_F(ReplicationDBTest, changeRole) {
  auto leader_db = createDb(laser::DBRole::LEADER);
  EXPECT_CALL(*leader_db, pullFromUpstream()).Times(1);
  leader_db->changeRole(laser::DBRole::LEADER);
  leader_db->changeRole(laser::DBRole::FOLLOWER);
}

TEST_F(ReplicationDBTest, delayPullFromUpstream) {
  auto db = createDb(laser::DBRole::LEADER);
  int64_t start_ms = static_cast<int64_t>(common::currentTimeInMs());
  int64_t exec_ms = 0;
  EXPECT_CALL(*db, pullFromUpstream()).Times(1).WillOnce(
      ::testing::InvokeWithoutArgs([&exec_ms]() { exec_ms = static_cast<int64_t>(common::currentTimeInMs()); }));
  db->delayPullFromUpstream();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_LE(error_delay_ms_, exec_ms - start_ms);
}

// 正常两个 db 进行数据同步
TEST_F(ReplicationDBTest, replicateSuccess) {
  auto leader_db = createDb(laser::DBRole::LEADER);
  auto follower_db = createDb(laser::DBRole::FOLLOWER);
  std::string data_prefix = "test";
  uint32_t number_batch = 100;

  // 先创建 100 batch, 然后 follower 进行同步，最终对 follower 数据进行校验
  writeBatchData(leader_db, data_prefix, number_batch);

  int64_t next_seq_no = 0;
  while (next_seq_no < number_batch) {
    replicateForward(&next_seq_no, leader_db, follower_db);
  }

  checkReadData(follower_db, data_prefix, number_batch);
}

TEST_F(ReplicationDBTest, getUpdatesFail) {
  auto leader_db = createDb(laser::DBRole::LEADER);
  auto follower_db = createDb(laser::DBRole::FOLLOWER);
  // 版本更新
  laser::ReplicateResponse response;
  laser::ReplicateRequest request;
  follower_db->getPullRequest(&request, laser::ReplicateType::FORWARD, 0);
  request.version = "test_version";
  leader_db->getUpdates(&response, std::make_unique<laser::ReplicateRequest>(request));
  EXPECT_EQ(version_, response.version);

  // 使用 follower 角色的 db 执行 getUpdates
  follower_db->getPullRequest(&request, laser::ReplicateType::FORWARD, 0);
  EXPECT_THROW({ follower_db->getUpdates(&response, std::make_unique<laser::ReplicateRequest>(request)); },
               laser::LaserException);
}

// 正常两个支持写批量同步
TEST_F(ReplicationDBTest, replicateBatchSuccess) {
  auto leader_db = createDb(laser::DBRole::LEADER);
  auto follower_db = createDb(laser::DBRole::FOLLOWER);
  std::string data_prefix = "test";
  uint32_t number_batch = 1001;
  uint32_t batch_size = 100;

  // 先创建 100 batch, 然后 follower 进行同步，最终对 follower 数据进行校验
  writeBatchDataEach(leader_db, data_prefix, number_batch, batch_size);

  int64_t next_seq_no = 0;
  while (next_seq_no < number_batch) {
    replicateForward(&next_seq_no, leader_db, follower_db);
  }

  checkReadData(follower_db, data_prefix, number_batch);
}
