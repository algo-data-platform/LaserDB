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

#include "common/laser/format.h"

TEST(LaserKeyFormat, packTest) {
  std::vector<std::string> primary_keys({"uid", "page"});
  std::vector<std::string> column_families({"age", "count"});
  laser::LaserKeyFormat to_buffer(primary_keys, column_families);
  EXPECT_EQ(40, to_buffer.length());
  EXPECT_EQ(4657409261488844495, to_buffer.getKeyHash());

  laser::LaserKeyFormat from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());

  auto from_primary_keys = from_buffer.getPrimaryKeys();
  auto from_column_families = from_buffer.getColumnFamilies();
  EXPECT_EQ("uid", from_primary_keys[0]);
  EXPECT_EQ("page", from_primary_keys[1]);
  EXPECT_EQ("age", from_column_families[0]);
  EXPECT_EQ("count", from_column_families[1]);
  EXPECT_EQ(laser::KeyType::DEFAULT, from_buffer.getKeyType());
}

TEST(LaserValueRawString, packTest) {
  laser::LaserValueRawString to_buffer("test");
  EXPECT_EQ(17, to_buffer.length());

  laser::LaserValueRawString from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());
  EXPECT_TRUE(from_buffer.decode());
  EXPECT_EQ("test", from_buffer.getValue());
  EXPECT_EQ(laser::ValueType::RAW_STRING, from_buffer.getType());

  from_buffer.clear();
  EXPECT_EQ(0, from_buffer.length());
  EXPECT_EQ("test", from_buffer.getValue());
  from_buffer.reset();
  EXPECT_EQ("", from_buffer.getValue());
}

TEST(LaserValueCounter, packTest) {
  laser::LaserValueCounter to_buffer(11111);
  EXPECT_EQ(17, to_buffer.length());

  laser::LaserValueCounter from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());
  EXPECT_EQ(11111, from_buffer.getValue());
  EXPECT_EQ(laser::ValueType::COUNTER, from_buffer.getType());
}

TEST(LaserValueCounter, packTestError) {
  laser::LaserValueCounter to_buffer(11111);
  EXPECT_EQ(17, to_buffer.length());

  laser::LaserValueRawString from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_FALSE(from_buffer.decode());
}

TEST(LaserValueZSet, packTest) {
  std::vector<std::string> zset_members({"member1", "member2"});
  laser::LaserValueZSet to_buffer(zset_members);
  EXPECT_EQ(35, to_buffer.length());

  laser::LaserValueZSet from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());
  auto from_members = from_buffer.getMembers();
  EXPECT_EQ("member1", from_members[0]);
  EXPECT_EQ("member2", from_members[1]);
  EXPECT_EQ(laser::ValueType::ZSET, from_buffer.getType());
}

TEST(LaserKeyFormatMapData, packTest) {
  std::vector<std::string> primary_keys({"uid", "page"});
  std::vector<std::string> column_families({"age", "count"});
  laser::LaserKeyFormat key(primary_keys, column_families);
  laser::LaserKeyFormatMapData to_buffer(key, "map_field");
  EXPECT_EQ(53, to_buffer.length());
  EXPECT_EQ(4657409261488844495, to_buffer.getKeyHash());

  laser::LaserKeyFormatMapData from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());

  auto from_primary_keys = from_buffer.getPrimaryKeys();
  auto from_column_families = from_buffer.getColumnFamilies();
  EXPECT_EQ("uid", from_primary_keys[0]);
  EXPECT_EQ("page", from_primary_keys[1]);
  EXPECT_EQ("age", from_column_families[0]);
  EXPECT_EQ("count", from_column_families[1]);
  EXPECT_EQ(laser::KeyType::COMPOSITE, from_buffer.getKeyType());
}

TEST(LaserKeyFormatZSetData, packTest) {
  std::vector<std::string> primary_keys({"uid", "page"});
  std::vector<std::string> column_families({"age", "count"});
  laser::LaserKeyFormat key(primary_keys, column_families);
  laser::LaserKeyFormatZSetData to_buffer(key, 1);
  EXPECT_EQ(48, to_buffer.length());
  EXPECT_EQ(4657409261488844495, to_buffer.getKeyHash());

  laser::LaserKeyFormatZSetData from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());

  auto from_primary_keys = from_buffer.getPrimaryKeys();
  auto from_column_families = from_buffer.getColumnFamilies();
  EXPECT_EQ("uid", from_primary_keys[0]);
  EXPECT_EQ("page", from_primary_keys[1]);
  EXPECT_EQ("age", from_column_families[0]);
  EXPECT_EQ("count", from_column_families[1]);
  EXPECT_EQ(laser::KeyType::COMPOSITE, from_buffer.getKeyType());
}

TEST(LaserKeyFormatSetData, packTest) {
  std::vector<std::string> primary_keys({"uid", "page"});
  std::vector<std::string> column_families({"age", "count"});
  laser::LaserKeyFormat key(primary_keys, column_families);
  laser::LaserKeyFormatSetData to_buffer(key, "member1");
  EXPECT_EQ(51, to_buffer.length());
  EXPECT_EQ(4657409261488844495, to_buffer.getKeyHash());

  laser::LaserKeyFormatSetData from_buffer(to_buffer.data(), to_buffer.length());
  EXPECT_TRUE(from_buffer.decode());

  auto from_primary_keys = from_buffer.getPrimaryKeys();
  auto from_column_families = from_buffer.getColumnFamilies();
  EXPECT_EQ("uid", from_primary_keys[0]);
  EXPECT_EQ("page", from_primary_keys[1]);
  EXPECT_EQ("age", from_column_families[0]);
  EXPECT_EQ("count", from_column_families[1]);
  EXPECT_EQ(laser::KeyType::COMPOSITE, from_buffer.getKeyType());
  EXPECT_EQ("member1", from_buffer.getData());
}
