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

#include "common/laser/config_manager.h"
#include "folly/FileUtil.h"

class MockRouter : public service_router::Router {
 public:
  MockRouter() : service_router::Router() {}
  MOCK_METHOD2(waitForConfig, void(const std::string&, bool));
  MOCK_METHOD1(getConfigs, const std::unordered_map<std::string, std::string>(const std::string& service_name));
  void subscribeConfig(const std::string&, service_router::NotifyConfig) override{};
};

class ConfigManagerTest : public ::testing::Test {
 public:
  ConfigManagerTest() {
    folly::SingletonVault::singleton()->registrationComplete();
    router_ = std::make_shared<MockRouter>();

    std::vector<std::string> config_items(
        {table_config_key_, cluster_config_key_, node_config_list_, table_config_list_, rocksdb_node_configs_});
    for (auto& key : config_items) {
      std::string config_info;
      readJsonFile(&config_info, key);
      origin_configs_[key] = config_info;
    }
  }
  virtual ~ConfigManagerTest() = default;
  virtual void TearDown() {}
  virtual void SetUp() { config_ = std::make_shared<laser::ConfigManager>(router_); }

 protected:
  std::shared_ptr<laser::ConfigManager> config_;
  std::shared_ptr<MockRouter> router_;
  std::string service_name_ = "test_service";
  std::string group_name_ = "test";
  uint32_t node_id_ = 1;
  std::string table_config_key_ = "database_table_schema_data";
  std::string cluster_config_key_ = "cluster_info_data";
  std::string node_config_list_ = "node_config_list_data";
  std::string table_config_list_ = "table_config_list_data";
  std::string rocksdb_node_configs_ = "rocksdb_node_configs_data";
  std::unordered_map<std::string, std::string> origin_configs_;

  void triggerUpdate(const std::unordered_map<std::string, std::string> configs, const std::string& group_name,
                     uint32_t node_id) {
    config_->updateConfig(configs, group_name, node_id);
  }

  void readJsonFile(std::string* data, const std::string& key) {
    std::string result;
    std::string file_name = key + ".json";
    folly::readFile(file_name.data(), result);
    *data = result;
  }
};

TEST_F(ConfigManagerTest, init) {
  LOG(INFO) << "TEST_F init";
  EXPECT_CALL(*router_, waitForConfig(::testing::Eq(service_name_), ::testing::Eq(true))).Times(1);
  EXPECT_CALL(*router_, getConfigs(::testing::Eq(service_name_))).Times(1).WillOnce(::testing::Return(origin_configs_));
  config_->init(service_name_, group_name_, node_id_);
}

TEST_F(ConfigManagerTest, updateDatabase) {
  triggerUpdate(origin_configs_, group_name_, node_id_);

  auto table = config_->getTableSchema("test", "test_string_set");
  EXPECT_TRUE(table.hasValue());
  EXPECT_EQ("test", table.value()->getDatabaseName());
  EXPECT_EQ("test_string_set", table.value()->getTableName());
  EXPECT_EQ(10, table.value()->getPartitionNumber());

  auto table_none = config_->getTableSchema("test", "user_info1");
  EXPECT_FALSE(table_none.hasValue());
}

TEST_F(ConfigManagerTest, updateClusterInfo) {
  triggerUpdate(origin_configs_, group_name_, node_id_);

  auto node = config_->getNodeShardList("aliyun", 1);
  EXPECT_TRUE(node.hasValue());
  EXPECT_EQ(1, node.value().getNodeId());
  EXPECT_THAT(std::vector<uint32_t>({0, 1, 2, 3, 4}), ::testing::ContainerEq(node.value().getLeaderShardList()));
  EXPECT_THAT(std::vector<uint32_t>({5, 6, 7, 8, 9}), ::testing::ContainerEq(node.value().getFollowerShardList()));

  auto node_none = config_->getNodeShardList("aliyun", 3);
  EXPECT_FALSE(node_none.hasValue());
}

TEST_F(ConfigManagerTest, updateRocksDbConfig) {
  triggerUpdate(origin_configs_, group_name_, node_id_);
  config_->subscribeRocksDbConfig([](std::shared_ptr<laser::NodeConfig> node_config,
                                     const laser::TableConfigList& table_config_list,
                                     const laser::TableSchemasMap& table_schemas) {
    EXPECT_EQ(1, node_config->getWriteBufferSizeGb());
    EXPECT_EQ(2, node_config->getBlockCacheSizeGb());
    EXPECT_EQ(3, table_config_list.getTableConfigList().size());
    EXPECT_EQ(1, table_config_list.getTableConfigList().at("default").getCfOptions().size());
    EXPECT_EQ(1, table_config_list.getTableConfigList().at("default").getDbOptions().size());
    EXPECT_EQ(1, table_config_list.getTableConfigList().at("default").getTableOptions().size());
    EXPECT_EQ(1, table_schemas.size());
  });
}

TEST_F(ConfigManagerTest, upateDatabaseAndCluster) {
  int count_1 = 0;
  config_->subscribe(
      "aliyun", 1,
      [&count_1](const laser::NodeShardList& list,
                 const std::unordered_map<uint64_t, std::shared_ptr<laser::TableSchema>>& tables) {
        count_1++;
        EXPECT_THAT(std::vector<uint32_t>({0, 1, 2, 3, 4}), ::testing::ContainerEq(list.getLeaderShardList()));
        EXPECT_THAT(std::vector<uint32_t>({5, 6, 7, 8, 9}), ::testing::ContainerEq(list.getFollowerShardList()));
        EXPECT_EQ(1, tables.size());
      });
  int count_3 = 0;
  // not exists
  config_->subscribe(
      "aliyun", 3,
      [&count_3](const laser::NodeShardList&,
                 const std::unordered_map<uint64_t, std::shared_ptr<laser::TableSchema>>&) { count_3++; });
  triggerUpdate(origin_configs_, group_name_, node_id_);
  EXPECT_EQ(0, count_3);
  EXPECT_EQ(1, count_1);
}
