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
#include "laser/server/laser_service.h"

DECLARE_string(vmodule);
class MockRocksDbEngine : public laser::RocksDbEngine {
 public:
  MockRocksDbEngine() : laser::RocksDbEngine(nullptr) {}
  MOCK_METHOD2(get, laser::Status(laser::LaserValueRawString* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(get, laser::Status(laser::LaserValueCounter* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(get, laser::Status(laser::LaserValueListMeta* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(get, laser::Status(laser::LaserValueSetMeta* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(set, laser::Status(const laser::LaserKeyFormat& key, const std::string& data));
  MOCK_METHOD2(mset,
               laser::Status(const std::vector<laser::LaserKeyFormat>& keys, const std::vector<std::string>& data));
  MOCK_METHOD3(hget, laser::Status(laser::LaserValueRawString* value, const laser::LaserKeyFormat& key,
                                   const std::string& field));
  MOCK_METHOD2(hgetall, laser::Status(std::unordered_map<std::string, laser::LaserValueRawString>* values,
                                      const laser::LaserKeyFormat& key));
  MOCK_METHOD3(decr, laser::Status(int64_t* value, const laser::LaserKeyFormat& key, uint64_t step));
  MOCK_METHOD3(incr, laser::Status(int64_t* value, const laser::LaserKeyFormat& key, uint64_t step));
  MOCK_METHOD2(hexists, laser::Status(const laser::LaserKeyFormat& key, const std::string& field));
  MOCK_METHOD3(hset,
               laser::Status(const laser::LaserKeyFormat& key, const std::string& field, const std::string& value));
  MOCK_METHOD2(hkeys, laser::Status(std::vector<laser::LaserKeyFormatMapData>* keys, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(hlen, laser::Status(laser::LaserValueMapMeta* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD3(lindex,
               laser::Status(laser::LaserValueRawString* value, const laser::LaserKeyFormat& key, int64_t index));
  MOCK_METHOD2(llen, laser::Status(laser::LaserValueListMeta* value, const laser::LaserKeyFormat& key));
  MOCK_METHOD4(lrange, laser::Status(std::vector<laser::LaserValueRawString>* values, const laser::LaserKeyFormat& key,
                                     uint64_t start, uint64_t end));
  MOCK_METHOD2(sadd, laser::Status(const laser::LaserKeyFormat& key, const std::string& member));
  MOCK_METHOD2(hasMember, laser::Status(const laser::LaserKeyFormat& key, const std::string& member));
  MOCK_METHOD2(sdel, laser::Status(const laser::LaserKeyFormat& key, const std::string& member));
  MOCK_METHOD2(members,
               laser::Status(std::vector<laser::LaserKeyFormatSetData>* members, const laser::LaserKeyFormat& key));
  MOCK_METHOD2(zadd,
               laser::Status(const laser::LaserKeyFormat& key, const std::map<std::string, int64_t>& member_scores));
  MOCK_METHOD4(zrangeByScore, laser::Status(std::vector<laser::LaserScoreMember>* score_members,
                                            const laser::LaserKeyFormat& key, int64_t min, int64_t max));
  MOCK_METHOD4(zremRangeByScore,
               laser::Status(int64_t* number, const laser::LaserKeyFormat& key, int64_t min, int64_t max));
};

class MockConfigManager : public laser::ConfigManager {
 public:
  MockConfigManager() : laser::ConfigManager(service_router::Router::getInstance()) {}
  MOCK_METHOD0(getTrafficRestrictionConfig, std::shared_ptr<laser::TableTrafficRestrictionMap>());
  MOCK_METHOD2(getTableSchemaHash, uint64_t(const std::string&, const std::string&));
};

class MockLaserService : public laser::LaserService {
 public:
  explicit MockLaserService(std::shared_ptr<laser::ConfigManager> config_manager)
      : laser::LaserService(config_manager, nullptr) {}
  MOCK_METHOD3(getDatabaseEngine, void(std::shared_ptr<laser::RocksDbEngine>*, const std::unique_ptr<laser::LaserKey>&,
                                       std::shared_ptr<laser::LaserKeyFormat>));
};

class LaserServiceTest : public ::testing::Test {
 public:
  LaserServiceTest() {
    // metrics 依赖单例
    folly::SingletonVault::singleton()->registrationComplete();
    FLAGS_vmodule = "laser_service=5";
    config_manager_ = std::make_shared<MockConfigManager>();
    service_ = std::make_shared<MockLaserService>(config_manager_);
  }
  virtual ~LaserServiceTest() = default;
  virtual void TearDown() {}
  virtual void SetUp() {
    db_engine_ = std::make_shared<MockRocksDbEngine>();

    EXPECT_CALL(*config_manager_, getTrafficRestrictionConfig()).WillRepeatedly(::testing::Return(nullptr));
    EXPECT_CALL(*config_manager_, getTableSchemaHash(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(0));
  }

  virtual std::unique_ptr<laser::LaserKey> createLaserKey() {
    std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>();
    key->set_database_name(database_name_);
    key->set_table_name(table_name_);
    key->set_primary_keys({"foo1"});
    key->set_column_keys({"bar1"});
    return std::move(key);
  }

  virtual std::unique_ptr<laser::LaserKV> createLaserKeyValue() {
    std::unique_ptr<laser::LaserKV> kv = std::make_unique<laser::LaserKV>();
    laser::LaserKey key;
    laser::LaserValue value;
    value.set_string_value(value_);
    kv->set_key(*createLaserKey());
    kv->set_value(std::move(value));
    return std::move(kv);
  }

 protected:
  std::shared_ptr<MockRocksDbEngine> db_engine_;
  std::shared_ptr<MockConfigManager> config_manager_;
  std::shared_ptr<MockLaserService> service_;
  std::string database_name_{"test"};
  std::string table_name_{"user"};
  std::string value_{"test"};
};

TEST_F(LaserServiceTest, get) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));

  EXPECT_CALL(*db_engine_, get(::testing::Matcher<laser::LaserValueRawString*>(::testing::_), ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  EXPECT_NO_THROW({ service_->get(response, createLaserKey()); });
  EXPECT_THROW({ service_->get(response, createLaserKey()); }, laser::LaserException);
}

TEST_F(LaserServiceTest, sset) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));
  EXPECT_CALL(*db_engine_, set(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_ERROR));

  laser::LaserResponse response;
  EXPECT_NO_THROW({ service_->sset(response, createLaserKeyValue()); });
  EXPECT_EQ(value_.size(), response.get_int_data());
  EXPECT_THROW({ service_->sset(response, createLaserKeyValue()); }, laser::LaserException);
}

TEST_F(LaserServiceTest, mget) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(4)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));

  laser::LaserValueRawString raw_string(value_);
  EXPECT_CALL(*db_engine_, get(::testing::Matcher<laser::LaserValueRawString*>(::testing::_), ::testing::_))
      .Times(4)
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND))
      .WillRepeatedly(::testing::DoAll(::testing::SetArgPointee<0>(raw_string), ::testing::Return(laser::Status::OK)));

  laser::LaserResponse response;
  std::unique_ptr<laser::LaserKeys> keys = std::make_unique<laser::LaserKeys>();
  std::vector<laser::LaserKey> vector_key;
  for (auto i = 0; i < 4; i++) {
    vector_key.push_back(*createLaserKey());
  }
  keys->set_keys(std::move(vector_key));
  service_->mget(response, std::move(keys));
  auto result_list = response.get_list_value_data();
  uint32_t i = 0;
  for (auto res : result_list) {
    if (i == 0) {
      EXPECT_EQ(true, res.get_null_value());
    } else {
      EXPECT_EQ(value_, res.get_string_value());
    }
    i++;
  }
}

TEST_F(LaserServiceTest, mset) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(4)
      .WillOnce(::testing::SetArgPointee<0>(nullptr))
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));
  EXPECT_CALL(*db_engine_, mset(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(laser::Status::OK));

  laser::LaserResponse response;
  std::unique_ptr<laser::LaserKVs> values = std::make_unique<laser::LaserKVs>();
  std::vector<laser::LaserKV> vector_kv;
  for (auto i = 0; i < 4; i++) {
    vector_kv.push_back(*createLaserKeyValue());
  }
  values->set_values(std::move(vector_kv));

  service_->mset(response, std::move(values));
  auto result_list = response.get_list_int_data();
  for (size_t i = 0; i < result_list.size(); i++) {
    if (i == 3) {
      EXPECT_EQ(-1, result_list[i]);
    } else {
      EXPECT_EQ(i, result_list[i]);
    }
  }
}

TEST_F(LaserServiceTest, hget) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));
  EXPECT_CALL(*db_engine_,
              hget(::testing::Matcher<laser::LaserValueRawString*>(::testing::_), ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  std::string test_field = "test_field";
  laser::LaserResponse response;
  EXPECT_NO_THROW({ service_->hget(response, createLaserKey(), std::make_unique<std::string>(test_field)); });
  EXPECT_THROW({ service_->hget(response, createLaserKey(), std::make_unique<std::string>(test_field)); },
               laser::LaserException);
}

TEST_F(LaserServiceTest, hgetall) {
  std::string field_prefix = "test";
  std::unordered_map<std::string, laser::LaserValueRawString> values;
  for (int i = 0; i < 10; i++) {
    laser::LaserValueRawString value(folly::to<std::string>(field_prefix, i));
    values[folly::to<std::string>(field_prefix, i)] = value;
  }

  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));

  EXPECT_CALL(*db_engine_,
              hgetall(::testing::Matcher<std::unordered_map<std::string, laser::LaserValueRawString>*>(::testing::_),
                      ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND))
      .WillRepeatedly(::testing::DoAll(::testing::SaveArgPointee<0>(&values), ::testing::Return(laser::Status::OK)));

  laser::LaserResponse response;
  EXPECT_THROW({ service_->hgetall(response, createLaserKey()); }, laser::LaserException);
  service_->hgetall(response, createLaserKey());
}

TEST_F(LaserServiceTest, zadd) {
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(1)
      .WillOnce(::testing::SetArgPointee<0>(db_engine_));

  EXPECT_CALL(*db_engine_, zadd(::testing::_, ::testing::_)).Times(1).WillOnce(::testing::Return(laser::Status::OK));

  laser::LaserResponse response;
  std::map<std::string, int64_t> member_score_map;
  member_score_map.insert({"one", 1});
  member_score_map.insert({"two", 2});

  std::unique_ptr<laser::LaserValue> member_scores = std::make_unique<laser::LaserValue>();
  member_scores->set_member_score_value(member_score_map);
  EXPECT_NO_THROW(service_->zadd(response, createLaserKey(), std::move(member_scores)));
  EXPECT_EQ(2, response.get_int_data());
}

TEST_F(LaserServiceTest, zrangeByScore) {
  std::vector<laser::LaserScoreMember> score_members;
  laser::LaserScoreMember score_member;
  score_member.set_score(1);
  score_member.set_member("one");
  score_members.push_back(score_member);
  score_member.set_score(2);
  score_member.set_member("two");
  score_members.push_back(score_member);
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));

  EXPECT_CALL(*db_engine_, zrangeByScore(::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillRepeatedly(
          ::testing::DoAll(::testing::SetArgPointee<0>(score_members), ::testing::Return(laser::Status::OK)));

  int64_t min = 1;
  int64_t max = 3;

  laser::LaserResponse response;
  std::unique_ptr<std::vector<laser::LaserScoreMember>> unique_score_members =
      std::make_unique<std::vector<laser::LaserScoreMember>>();
  EXPECT_NO_THROW(service_->zrangeByScore(response, createLaserKey(), min, max));
  service_->zrangeByScore(response, createLaserKey(), min, max);
  auto res_score_members = response.get_list_score_member_data();
  EXPECT_EQ(1, res_score_members[0].get_score());
  EXPECT_EQ("one", res_score_members[0].get_member());
  EXPECT_EQ(2, res_score_members[1].get_score());
  EXPECT_EQ("two", res_score_members[1].get_member());
}

TEST_F(LaserServiceTest, zremRangeByScore) {
  int64_t number = 3;
  EXPECT_CALL(*service_,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::SetArgPointee<0>(db_engine_));

  EXPECT_CALL(*db_engine_, zremRangeByScore(::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillRepeatedly(::testing::DoAll(::testing::SetArgPointee<0>(number), ::testing::Return(laser::Status::OK)));

  int64_t min = 1;
  int64_t max = 3;
  laser::LaserResponse response;
  std::unique_ptr<std::vector<laser::LaserScoreMember>> unique_score_members =
      std::make_unique<std::vector<laser::LaserScoreMember>>();
  EXPECT_NO_THROW(service_->zremRangeByScore(response, createLaserKey(), min, max));
  service_->zremRangeByScore(response, createLaserKey(), min, max);
  EXPECT_EQ(3, response.get_int_data());
}

/*
TEST(LaserService, decr) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, decr(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.decr(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.decr(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.decr(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, counter_incr) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, incr(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.incr(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.incr(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.incr(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, hexists) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));
  EXPECT_CALL(*db_engine,
              hget(::testing::Matcher<laser::LaserValueRawString*>(::testing::_), ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  std::string st = "field";
  std::unique_ptr<std::string> field = std::make_unique<std::string>(std::move(st));

  service.hget(response, std::move(key), std::move(field));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  std::string st2 = "field";
  std::unique_ptr<std::string> field2 = std::make_unique<std::string>(std::move(st2));
  service.hget(response, std::move(key2), std::move(field2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  std::string st3 = "field";
  std::unique_ptr<std::string> field3 = std::make_unique<std::string>(std::move(st3));
  service.hget(response, std::move(key3), std::move(field3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, hset) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, hset(::testing::_, ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  std::string st = "field";
  std::unique_ptr<std::string> field = std::make_unique<std::string>(std::move(st));
  std::string val = "value";
  std::unique_ptr<std::string> value = std::make_unique<std::string>(std::move(val));

  service.hset(response, std::move(key), std::move(field), std::move(value));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  std::string st2 = "field";
  std::unique_ptr<std::string> field2 = std::make_unique<std::string>(std::move(st2));
  std::string val2 = "value";
  std::unique_ptr<std::string> value2 = std::make_unique<std::string>(std::move(val2));

  service.hset(response, std::move(key2), std::move(field2), std::move(value2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  std::string st3 = "field";
  std::unique_ptr<std::string> field3 = std::make_unique<std::string>(std::move(st3));
  std::string val3 = "value";
  std::unique_ptr<std::string> value3 = std::make_unique<std::string>(std::move(val3));

  service.hset(response, std::move(key3), std::move(field3), std::move(value3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, hkeys) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine,
              hkeys(::testing::Matcher<std::vector<laser::LaserKeyFormatMapData>*>(::testing::_), ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.hkeys(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.hkeys(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.hkeys(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, hlen) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, hlen(::testing::Matcher<laser::LaserValueMapMeta*>(::testing::_), ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.hlen(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.hlen(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.hlen(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, lindex) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine,
              lindex(::testing::Matcher<laser::LaserValueRawString*>(::testing::_), ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));

  int32_t index = 1;
  service.lindex(response, std::move(key), index);
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.lindex(response, std::move(key2), index);
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.lindex(response, std::move(key3), index);
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, llen) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, llen(::testing::Matcher<laser::LaserValueListMeta*>(::testing::_), ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.llen(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.llen(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.llen(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, lrange) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, lrange(::testing::Matcher<std::vector<laser::LaserValueRawString>*>(::testing::_),
                                 ::testing::_, ::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  uint64_t start = 0;
  uint64_t end = 1;
  service.lrange(response, std::move(key), start, end);
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.lrange(response, std::move(key2), start, end);
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.lrange(response, std::move(key3), start, end);
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, sadd) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, sadd(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  std::string meb = "member";
  std::unique_ptr<std::string> meb_ptr = std::make_unique<std::string>(std::move(meb));
  service.sadd(response, std::move(key), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.sadd(response, std::move(key2), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.sadd(response, std::move(key3), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, sismember) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, hasMember(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::string meb = "member";
  std::unique_ptr<std::string> meb_ptr = std::make_unique<std::string>(std::move(meb));
  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.sismember(response, std::move(key), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.sismember(response, std::move(key2), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.sismember(response, std::move(key3), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, sdel) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine, sdel(::testing::_, ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::string meb = "member";
  std::unique_ptr<std::string> meb_ptr = std::make_unique<std::string>(std::move(meb));
  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.sremove(response, std::move(key), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.sremove(response, std::move(key2), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.sremove(response, std::move(key3), std::move(meb_ptr));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}

TEST(LaserService, smembers) {
  MockLaserService service;
  laser::RocksDbConfig config;
  std::shared_ptr<MockRocksDbEngine> db_engine = std::make_shared<MockRocksDbEngine>(config);

  std::string database_name = "test";
  std::string table_name = "user";

  EXPECT_CALL(service,
              getDatabaseEngine(::testing::_, ::testing::Matcher<const std::unique_ptr<laser::LaserKey>&>(::testing::_),
                                ::testing::_))
      .Times(3)
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(false)))
      .WillOnce(::testing::DoAll(::testing::SetArgPointee<0>(db_engine), ::testing::Return(true)));

  EXPECT_CALL(*db_engine,
              members(::testing::Matcher<std::vector<laser::LaserKeyFormatSetData>*>(::testing::_), ::testing::_))
      .Times(2)
      .WillOnce(::testing::Return(laser::Status::OK))
      .WillOnce(::testing::Return(laser::Status::RS_NOT_FOUND));

  laser::LaserResponse response;
  laser::LaserKey laser_key;
  laser_key.set_database_name(database_name);
  laser_key.set_table_name(table_name);
  laser_key.set_primary_keys({"foo"});
  laser_key.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key = std::make_unique<laser::LaserKey>(std::move(laser_key));
  service.smembers(response, std::move(key));
  EXPECT_EQ(laser::LaserResponseCode::OK, response.get_code());

  laser::LaserKey laser_key2;
  laser_key2.set_database_name(database_name);
  laser_key2.set_table_name(table_name);
  laser_key2.set_primary_keys({"foo"});
  laser_key2.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key2 = std::make_unique<laser::LaserKey>(std::move(laser_key2));
  service.smembers(response, std::move(key2));
  EXPECT_EQ(laser::LaserResponseCode::NOT_EXISTS_PARTITION, response.get_code());

  laser::LaserKey laser_key3;
  laser_key3.set_database_name(database_name);
  laser_key3.set_table_name(table_name);
  laser_key3.set_primary_keys({"foo"});
  laser_key3.set_column_keys({"bar"});

  std::unique_ptr<laser::LaserKey> key3 = std::make_unique<laser::LaserKey>(std::move(laser_key3));
  service.smembers(response, std::move(key3));
  EXPECT_EQ(laser::LaserResponseCode::ENGINE_ERROR, response.get_code());
}
*/
