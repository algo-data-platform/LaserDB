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

#include "laser/server/engine/rocksdb.h"

#include "boost/filesystem.hpp"

#include "folly/Singleton.h"
#include "folly/portability/GTest.h"
#include "folly/Random.h"

namespace laser {

DECLARE_bool(batch_ingest_format_is_sst);

namespace test {

class RocksdbTest : public ::testing::Test {
 public:
  RocksdbTest() {
    folly::SingletonVault::singleton()->registrationComplete();
    path_ = folly::to<std::string>("/tmp/rocksdb_unit_test_", folly::Random::secureRand32());

    std::vector<std::string> primary_keys({"uid", "xx"});
    std::vector<std::string> column_names({"age"});
    LaserKeyFormat test_key(primary_keys, column_names);
    key_ = test_key;
    options_.create_if_missing = true;

    // 默认仅测试加载 sst 格式文件
    laser::FLAGS_batch_ingest_format_is_sst = true;
  }

  virtual ~RocksdbTest() {
    boost::filesystem::path bpath(path_);
    if (boost::filesystem::exists(bpath)) {
      boost::filesystem::remove_all(bpath);
    }
  }

  bool opendb() {
    auto db = std::make_shared<laser::ReplicationDB>(path_, options_);
    db_ = std::make_shared<laser::RocksDbEngine>(db);
    return db_->open();
  }

  virtual void TearDown() {
    boost::filesystem::path bpath(path_);
    if (boost::filesystem::exists(bpath)) {
      boost::filesystem::remove_all(bpath);
    }
  }

  virtual void SetUp() {}

 protected:
  rocksdb::Options options_;
  std::string path_;
  std::shared_ptr<laser::RocksDbEngine> db_;
  LaserKeyFormat key_;

  void testIngestDeltaStringData(uint32_t size) {
    EXPECT_TRUE(opendb());
    std::string sst_file_temp_db = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
    auto replication_db = std::make_shared<laser::ReplicationDB>(sst_file_temp_db, options_);
    auto dump_db = std::make_shared<laser::RocksDbEngine>(replication_db);
    EXPECT_TRUE(dump_db->open());

    std::string value_prefix = "xxxx";

    for (uint32_t i = 0; i < size; i++) {
      std::string data = folly::to<std::string>(value_prefix, i);
      std::vector<std::string> primary_keys({"uid", "xx"});
      std::vector<std::string> column_names({data});
      LaserKeyFormat test_key(primary_keys, column_names);
      laser::Status status = dump_db->set(test_key, data);
      EXPECT_EQ(laser::Status::OK, status);
    }
    std::string sst_file_path = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
    laser::Status status = dump_db->dumpSst(sst_file_path);
    EXPECT_EQ(laser::Status::OK, status);

    std::string tempdb_path = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
    status = db_->ingestDeltaSst(sst_file_path, tempdb_path);
    EXPECT_EQ(laser::Status::OK, status);

    for (uint32_t i = 0; i < size; i++) {
      std::string data = folly::to<std::string>(value_prefix, i);
      std::vector<std::string> primary_keys({"uid", "xx"});
      std::vector<std::string> column_names({data});
      LaserKeyFormat test_key(primary_keys, column_names);
      laser::LaserValueRawString value;
      laser::Status status = db_->get(&value, test_key);
      EXPECT_EQ(laser::Status::OK, status);
      EXPECT_EQ(data, value.getValue());
    }
  }
};

TEST_F(RocksdbTest, opendb) { EXPECT_TRUE(opendb()); }

TEST_F(RocksdbTest, append) {
  EXPECT_TRUE(opendb());

  uint32_t length = 0;
  laser::Status s = db_->append(&length, key_, "data");
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(4, length);

  laser::LaserValueRawString value;
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("data", value.getValue());

  length = 0;
  s = db_->append(&length, key_, "111");
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(7, length);

  value.reset();
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("data111", value.getValue());
}

TEST_F(RocksdbTest, exist) {
  EXPECT_TRUE(opendb());

  uint32_t length = 0;
  laser::Status s = db_->append(&length, key_, "data");
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(4, length);

  laser::LaserValueRawString value;
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("data", value.getValue());

  bool has_key = false;
  s = db_->exist(&has_key, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(true, has_key);

  s = db_->delkey(key_);
  EXPECT_EQ(laser::Status::OK, s);

  s = db_->exist(&has_key, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(false, has_key);
}

TEST_F(RocksdbTest, setx) {
  EXPECT_TRUE(opendb());

  RocksDbEngineSetOptions options;
  options.not_exists = true;
  laser::Status s = db_->setx(key_, "data", options);
  EXPECT_EQ(laser::Status::OK, s);

  laser::LaserValueRawString value;
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("data", value.getValue());

  s = db_->setx(key_, "data", options);
  EXPECT_EQ(laser::Status::RS_KEY_EXISTS, s);

  s = db_->delkey(key_);
  EXPECT_EQ(laser::Status::OK, s);

  options.ttl = 5;
  options.not_exists = true;
  s = db_->setx(key_, "data", options);
  EXPECT_EQ(laser::Status::OK, s);

  s = db_->setx(key_, "data", options);
  EXPECT_EQ(laser::Status::RS_KEY_EXISTS, s);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  s = db_->setx(key_, "data", options);
  EXPECT_EQ(laser::Status::OK, s);
}

TEST_F(RocksdbTest, set) {
  EXPECT_TRUE(opendb());

  laser::Status s = db_->set(key_, "data");
  EXPECT_EQ(laser::Status::OK, s);

  laser::LaserValueRawString value;
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("data", value.getValue());

  s = db_->set(key_, "111");
  EXPECT_EQ(laser::Status::OK, s);

  value.reset();
  s = db_->get(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ("111", value.getValue());
}

TEST_F(RocksdbTest, incrAndDecr) {
  EXPECT_TRUE(opendb());
  std::vector<std::string> primary_keys({"uid", "xx"});
  std::vector<std::string> column_names({"age"});
  int64_t value;

  laser::Status s = db_->incr(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, value);

  s = db_->incr(&value, key_, 1000);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1001, value);

  s = db_->decr(&value, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1000, value);

  s = db_->decr(&value, key_, 3000);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(-2000, value);
}

TEST_F(RocksdbTest, hset) {
  EXPECT_TRUE(opendb());
  std::string field = "test1";
  std::string field_value = "xxxx";
  laser::Status s = db_->hset(key_, field, field_value);
  EXPECT_EQ(laser::Status::OK, s);

  LaserValueMapMeta map_meta;
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, map_meta.getSize());

  LaserValueRawString value;
  s = db_->hget(&value, key_, field);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(field_value, value.getValue());

  // set 一个 存在的 key
  s = db_->hset(key_, field, field_value);
  EXPECT_EQ(laser::Status::OK, s);

  map_meta.reset();
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, map_meta.getSize());
}

TEST_F(RocksdbTest, hmset) {
  EXPECT_TRUE(opendb());
  std::string field_prefix = "test";
  std::string field_value = "xxxx";

  std::string field0 = "test0";
  laser::Status s = db_->hset(key_, field0, field_value);
  EXPECT_EQ(laser::Status::OK, s);

  LaserValueMapMeta map_meta;
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, map_meta.getSize());

  LaserValueRawString value;
  s = db_->hget(&value, key_, field0);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(field_value, value.getValue());

  std::map<std::string, std::string> data;
  for (int i = 0; i < 10; i++) {
    data[folly::to<std::string>(field_prefix, i)] = folly::to<std::string>(field_value, i);
  }
  // set 一个 存在的 key
  s = db_->hmset(key_, data);
  EXPECT_EQ(laser::Status::OK, s);

  map_meta.reset();
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(10, map_meta.getSize());

  std::vector<laser::LaserKeyFormatMapData> keys;
  s = db_->hkeys(&keys, key_);
  EXPECT_EQ(laser::Status::OK, s);

  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(folly::to<std::string>(field_prefix, i), keys[i].getField());
  }

  for (int i = 0; i < 10; i++) {
    std::string field = folly::to<std::string>(field_prefix, i);
    std::string value = folly::to<std::string>(field_value, i);
    LaserValueRawString raw_value;
    s = db_->hget(&raw_value, key_, field);
    EXPECT_EQ(laser::Status::OK, s);
    EXPECT_EQ(value, raw_value.getValue());
  }
}

TEST_F(RocksdbTest, hdel) {
  EXPECT_TRUE(opendb());

  std::string field = "test1";
  std::string field_value = "xxxx";
  laser::Status s = db_->hset(key_, field, field_value);
  EXPECT_EQ(laser::Status::OK, s);

  LaserValueMapMeta map_meta;
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, map_meta.getSize());

  s = db_->hdel(key_, field);
  EXPECT_EQ(laser::Status::OK, s);

  map_meta.reset();
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(0, map_meta.getSize());

  s = db_->hdel(key_, field);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, s);
}

TEST_F(RocksdbTest, hget) {
  EXPECT_TRUE(opendb());

  std::string field = "test";
  std::string field1 = "test1";
  std::string field_value = "xxxx";
  laser::Status s = db_->hset(key_, field, field_value);
  EXPECT_EQ(laser::Status::OK, s);

  LaserValueMapMeta map_meta;
  s = db_->hlen(&map_meta, key_);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, map_meta.getSize());

  LaserValueRawString value;
  s = db_->hget(&value, key_, field);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(field_value, value.getValue());

  s = db_->hget(&value, key_, field1);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, s);
}

TEST_F(RocksdbTest, hkeys) {
  EXPECT_TRUE(opendb());

  std::string field_prefix = "test1";
  std::string field_value = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status s = db_->hset(key_, folly::to<std::string>(field_prefix, i), field_value);
    EXPECT_EQ(laser::Status::OK, s);
  }

  std::vector<laser::LaserKeyFormatMapData> keys;
  laser::Status s = db_->hkeys(&keys, key_);
  EXPECT_EQ(laser::Status::OK, s);

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(folly::to<std::string>(field_prefix, i), keys[i].getField());
  }
}

TEST_F(RocksdbTest, hgetall) {
  EXPECT_TRUE(opendb());

  std::string field_prefix = "test1";
  std::string field_value = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status s = db_->hset(key_, folly::to<std::string>(field_prefix, i), field_value);
    EXPECT_EQ(laser::Status::OK, s);
  }

  std::unordered_map<std::string, LaserValueRawString> values;
  laser::Status s = db_->hgetall(&values, key_);
  EXPECT_EQ(laser::Status::OK, s);

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(field_value, values[folly::to<std::string>(field_prefix, i)].getValue());
  }
}

TEST_F(RocksdbTest, pushFront) {
  EXPECT_TRUE(opendb());

  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->pushFront(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  laser::LaserValueListMeta list_meta;
  laser::Status status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, list_meta.getSize());

  LaserValueRawString value;
  status = db_->lindex(&value, key_, 98);
  EXPECT_EQ(laser::Status::OK, status);
  // 99 98 .......0
  EXPECT_EQ(folly::to<std::string>(value_prefix, 1), value.getValue());

  value.reset();
  status = db_->lindex(&value, key_, -2);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(folly::to<std::string>(value_prefix, 1), value.getValue());

  value.reset();
  status = db_->lindex(&value, key_, 0);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(folly::to<std::string>(value_prefix, 99), value.getValue());

  value.reset();
  status = db_->lindex(&value, key_, 100);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);
}

TEST_F(RocksdbTest, pushBack) {
  EXPECT_TRUE(opendb());

  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->pushBack(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  laser::LaserValueListMeta list_meta;
  laser::Status status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, list_meta.getSize());

  LaserValueRawString value;
  status = db_->lindex(&value, key_, 98);
  EXPECT_EQ(laser::Status::OK, status);
  // 0 1 2 ..... 98 99
  EXPECT_EQ(folly::to<std::string>(value_prefix, 98), value.getValue());
}

TEST_F(RocksdbTest, popFront) {
  EXPECT_TRUE(opendb());

  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->pushBack(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  laser::LaserValueListMeta list_meta;
  laser::Status status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, list_meta.getSize());

  LaserValueRawString value;
  for (int i = 0; i < 100; i++) {
    value.reset();
    status = db_->popFront(&value, key_);
    EXPECT_EQ(laser::Status::OK, status);
    EXPECT_EQ(folly::to<std::string>(value_prefix, i), value.getValue());
  }

  list_meta.reset();
  status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(0, list_meta.getSize());
}

TEST_F(RocksdbTest, popBack) {
  EXPECT_TRUE(opendb());

  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->pushFront(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  laser::LaserValueListMeta list_meta;
  laser::Status status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, list_meta.getSize());

  LaserValueRawString value;
  for (int i = 0; i < 100; i++) {
    value.reset();
    status = db_->popBack(&value, key_);
    EXPECT_EQ(laser::Status::OK, status);
    EXPECT_EQ(folly::to<std::string>(value_prefix, i), value.getValue());
  }

  list_meta.reset();
  status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(0, list_meta.getSize());
}

TEST_F(RocksdbTest, popEmpty) {
  EXPECT_TRUE(opendb());

  laser::LaserValueListMeta list_meta;
  laser::Status status = db_->get(&list_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(0, list_meta.getSize());

  LaserValueRawString value;
  status = db_->popBack(&value, key_);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);

  status = db_->pushFront(key_, "test");
  EXPECT_EQ(laser::Status::OK, status);

  value.reset();
  status = db_->popBack(&value, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ("test", value.getValue());

  value.reset();
  status = db_->popBack(&value, key_);
  EXPECT_EQ(laser::Status::RS_EMPTY, status);
}

TEST_F(RocksdbTest, lrange) {
  EXPECT_TRUE(opendb());

  std::vector<LaserValueRawString> values;
  laser::Status status = db_->lrange(&values, key_);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);

  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->pushBack(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  values.clear();
  status = db_->lrange(&values, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, values.size());

  values.clear();
  status = db_->lrange(&values, key_, 4);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(96, values.size());
  for (int i = 4; i < 100; i++) {
    EXPECT_EQ(folly::to<std::string>(value_prefix, i), values[i - 4].getValue());
  }

  values.clear();
  status = db_->lrange(&values, key_, 4, 10);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(6, values.size());
  for (int i = 4; i < 10; i++) {
    EXPECT_EQ(folly::to<std::string>(value_prefix, i), values[i - 4].getValue());
  }

  values.clear();
  status = db_->lrange(&values, key_, 4, 3);
  EXPECT_EQ(laser::Status::RS_INVALID_ARGUMENT, status);

  values.clear();
  status = db_->lrange(&values, key_, 100, 200);
  EXPECT_EQ(laser::Status::RS_INVALID_ARGUMENT, status);
}

TEST_F(RocksdbTest, sadd) {
  EXPECT_TRUE(opendb());
  std::string value_prefix = "xxxx";
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->sadd(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  LaserValueSetMeta set_meta;
  laser::Status status = db_->get(&set_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, set_meta.getSize());

  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->sadd(key_, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  set_meta.reset();
  status = db_->get(&set_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, set_meta.getSize());

  std::vector<LaserKeyFormatSetData> members;
  status = db_->members(&members, key_);
  EXPECT_EQ(laser::Status::OK, status);

  for (int i = 0; i < 100; i++) {
    EXPECT_EQ(folly::to<std::string>(value_prefix, i), members[i].getData());
  }

  status = db_->hasMember(key_, folly::to<std::string>(value_prefix, 50));
  EXPECT_EQ(laser::Status::OK, status);

  status = db_->hasMember(key_, folly::to<std::string>(value_prefix, 500));
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);

  status = db_->sdel(key_, folly::to<std::string>(value_prefix, 50));
  EXPECT_EQ(laser::Status::OK, status);
  status = db_->hasMember(key_, folly::to<std::string>(value_prefix, 50));
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);

  set_meta.reset();
  status = db_->get(&set_meta, key_);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(99, set_meta.getSize());
}

TEST_F(RocksdbTest, zset) {
  EXPECT_TRUE(opendb());

  std::map<std::string, int64_t> member_scores;
  member_scores.insert({"negative_one_million", -1000000});
  member_scores.insert({"negative_two_million", -2000000});
  member_scores.insert({"three", 3});
  member_scores.insert({"four", 4});
  laser::Status s = db_->zadd(key_, member_scores);
  EXPECT_EQ(laser::Status::OK, s);

  // the same score and member should not add
  s = db_->zadd(key_, member_scores);
  EXPECT_EQ(laser::Status::OK, s);

  std::vector<LaserScoreMember> members;

  s = db_->zrangeByScore(&members, key_, -2000000, 0);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(2, members.size());
  EXPECT_EQ("negative_two_million", members[0].get_member());
  EXPECT_EQ(-2000000, members[0].get_score());
  EXPECT_EQ("negative_one_million", members[1].get_member());
  EXPECT_EQ(-1000000, members[1].get_score());

  members.clear();
  s = db_->zrangeByScore(&members, key_, -3000000, 4);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(4, members.size());
  EXPECT_EQ("negative_two_million", members[0].get_member());
  EXPECT_EQ(-2000000, members[0].get_score());
  EXPECT_EQ("negative_one_million", members[1].get_member());
  EXPECT_EQ(-1000000, members[1].get_score());
  EXPECT_EQ("three", members[2].get_member());
  EXPECT_EQ(3, members[2].get_score());
  EXPECT_EQ("four", members[3].get_member());
  EXPECT_EQ(4, members[3].get_score());

  members.clear();
  s = db_->zrangeByScore(&members, key_, 0, 4);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(2, members.size());
  EXPECT_EQ("three", members[0].get_member());
  EXPECT_EQ(3, members[0].get_score());
  EXPECT_EQ("four", members[1].get_member());
  EXPECT_EQ(4, members[1].get_score());

  std::map<std::string, int64_t> member_scores2;
  member_scores2.insert({"negative_one_million_second", -1000000});
  member_scores2.insert({"negative_two_million_second", -2000000});
  s = db_->zadd(key_, member_scores2);
  EXPECT_EQ(laser::Status::OK, s);

  int64_t number = 0;
  s = db_->zremRangeByScore(&number, key_, -1000000, -1000000);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(1, number);

  members.clear();
  s = db_->zrangeByScore(&members, key_, -2000000, -1000000);
  EXPECT_EQ(laser::Status::OK, s);
  EXPECT_EQ(2, members.size());
  EXPECT_EQ("negative_two_million", members[0].get_member());
  EXPECT_EQ(-2000000, members[0].get_score());
  EXPECT_EQ("negative_two_million_second", members[1].get_member());
  EXPECT_EQ(-2000000, members[1].get_score());

  uint64_t expire_time = 5 + static_cast<uint64_t>(common::currentTimeInMs());
  s = db_->expireAt(key_, expire_time);
  EXPECT_EQ(laser::Status::OK, s);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  s = db_->zrangeByScore(&members, key_, -2000000, -1000000);
  EXPECT_EQ(laser::Status::RS_KEY_EXPIRE, s);
}

TEST_F(RocksdbTest, type_error) {
  EXPECT_TRUE(opendb());
  std::vector<std::string> primary_keys({"uid", "xx"});
  std::vector<std::string> column_names({"age"});

  uint32_t length = 0;
  laser::Status s = db_->append(&length, key_, "data");
  EXPECT_EQ(laser::Status::OK, s);

  // 类型不对
  int64_t value;
  s = db_->incr(&value, key_);
  EXPECT_EQ(laser::Status::RS_INVALID_ARGUMENT, s);
}

TEST_F(RocksdbTest, delkey) {
  EXPECT_TRUE(opendb());
  std::string value_prefix = "xxxx";

  std::vector<std::string> set_pk({"uid", "set"});
  std::vector<std::string> column_names({"age"});
  LaserKeyFormat set_key(set_pk, column_names);
  for (int i = 0; i < 100; i++) {
    laser::Status status = db_->sadd(set_key, folly::to<std::string>(value_prefix, i));
    EXPECT_EQ(laser::Status::OK, status);
  }

  LaserValueSetMeta set_meta;
  laser::Status status = db_->get(&set_meta, set_key);
  EXPECT_EQ(laser::Status::OK, status);
  EXPECT_EQ(100, set_meta.getSize());

  status = db_->delkey(set_key);
  EXPECT_EQ(laser::Status::OK, status);

  status = db_->get(&set_meta, set_key);
  EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);
}

TEST_F(RocksdbTest, ingestDeltaData) {
  EXPECT_TRUE(opendb());

  std::string sst_file_temp_db = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
  auto replication_db = std::make_shared<laser::ReplicationDB>(sst_file_temp_db, options_);
  auto dump_db = std::make_shared<laser::RocksDbEngine>(replication_db);
  EXPECT_TRUE(dump_db->open());

  std::string value_prefix = "xxxx";

  std::vector<std::string> set_pk({"uid", "set"});
  std::vector<std::string> column_names({"age"});
  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    set_pk[1] = folly::to<std::string>("set", key_index);
    LaserKeyFormat set_key(set_pk, column_names);
    for (int i = 0; i < 100; i++) {
      laser::Status status = dump_db->sadd(set_key, folly::to<std::string>(value_prefix, i));
      EXPECT_EQ(laser::Status::OK, status);
    }
  }

  std::string sst_file_path = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
  laser::Status status = dump_db->dumpSst(sst_file_path);
  EXPECT_EQ(laser::Status::OK, status);

  std::string tempdb_path = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
  status = db_->ingestDeltaSst(sst_file_path, tempdb_path);
  EXPECT_EQ(laser::Status::OK, status);

  // check
  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    set_pk[1] = folly::to<std::string>("set", key_index);
    LaserKeyFormat set_key(set_pk, column_names);
    LaserValueSetMeta set_meta;
    laser::Status status = db_->get(&set_meta, set_key);
    EXPECT_EQ(laser::Status::OK, status);
    EXPECT_EQ(100, set_meta.getSize());

    std::vector<LaserKeyFormatSetData> members;
    status = db_->members(&members, set_key);
    EXPECT_EQ(laser::Status::OK, status);

    for (int i = 0; i < 100; i++) {
      EXPECT_EQ(folly::to<std::string>(value_prefix, i), members[i].getData());
    }
  }
}

TEST_F(RocksdbTest, ingestDeltaDataString) {
  testIngestDeltaStringData(100);
  testIngestDeltaStringData(1000);
  testIngestDeltaStringData(1001);
  testIngestDeltaStringData(10000);
}

TEST_F(RocksdbTest, expire) {
  EXPECT_TRUE(opendb());

  std::string sst_file_temp_db = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
  auto replication_db = std::make_shared<laser::ReplicationDB>(sst_file_temp_db, options_);
  auto dump_db = std::make_shared<laser::RocksDbEngine>(replication_db);
  EXPECT_TRUE(dump_db->open());

  std::string value_prefix = "xxxx";
  std::string key_prefix = "string";

  std::vector<std::string> pk({"uid", "0"});
  std::vector<std::string> column_names({"age"});
  uint64_t expire_time = 30000 + static_cast<uint64_t>(common::currentTimeInMs());
  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    pk[1] = folly::to<std::string>(key_prefix, key_index);
    LaserKeyFormat set_key(pk, column_names);
    laser::Status status = dump_db->set(set_key, folly::to<std::string>(value_prefix, key_index));
    EXPECT_EQ(laser::Status::OK, status);

    status = dump_db->expireAt(set_key, expire_time + key_index);
    EXPECT_EQ(laser::Status::OK, status);

    LaserValueRawString data;
    status = dump_db->get(&data, set_key);
    EXPECT_EQ(laser::Status::OK, status);
    EXPECT_EQ(folly::to<std::string>(value_prefix, key_index), data.getValue());
  }
}

TEST_F(RocksdbTest, deleteExpireKeysAndAutoExpire) {
  std::string test_path = folly::to<std::string>(path_, "/", folly::Random::secureRand32());
  auto replication_db = std::make_shared<laser::ReplicationDB>(test_path, options_);

  auto rocks_options = std::make_shared<RocksDbEngineOptions>();
  rocks_options->ttl = 5;
  auto db = std::make_shared<laser::RocksDbEngine>(replication_db, rocks_options);

  EXPECT_TRUE(db->open());

  std::string value_prefix = "xxxx";
  std::string key_prefix = "string";

  std::vector<std::string> pk({"uid", "0"});
  std::vector<std::string> column_names({"age"});
  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    pk[1] = folly::to<std::string>(key_prefix, key_index);
    LaserKeyFormat set_key(pk, column_names);
    laser::Status status = db->set(set_key, folly::to<std::string>(value_prefix, key_index));
    EXPECT_EQ(laser::Status::OK, status);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    pk[1] = folly::to<std::string>(key_prefix, key_index);
    LaserKeyFormat set_key(pk, column_names);
    LaserValueRawString data;
    Status status = db->get(&data, set_key);
    EXPECT_EQ(laser::Status::RS_KEY_EXPIRE, status);
  }

  replication_db->compactRange();

  for (uint32_t key_index = 0; key_index < 100; key_index++) {
    pk[1] = folly::to<std::string>(key_prefix, key_index);
    LaserKeyFormat set_key(pk, column_names);
    LaserValueRawString data;
    Status status = db->get(&data, set_key);
    EXPECT_EQ(laser::Status::RS_NOT_FOUND, status);
  }
}

}  // namespace test
}  // namespace laser
