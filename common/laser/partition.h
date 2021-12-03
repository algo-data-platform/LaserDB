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
 * @author liubang <it.liubang@gmail.com>
 */

#pragma once

#include "city.h"

#include "config_manager.h"
#include "folly/Synchronized.h"
#include "format.h"
#include "laser_entity.h"

namespace laser {

class Partition {
 public:
  Partition(const std::string& database_name, const std::string& table_name, uint32_t partition_id)
      : database_name_(database_name), table_name_(table_name), partition_id_(partition_id) {}

  ~Partition() = default;

  inline const std::string& getDatabaseName() const { return database_name_; }

  inline const std::string& getTableName() const { return table_name_; }

  inline uint32_t getPartitionId() const { return partition_id_; }

  inline uint32_t getSrcShardId() const { return src_shard_id_; }

  inline void setSrcShardId(uint32_t src_shard_id) { src_shard_id_ = src_shard_id; }

  inline uint32_t getShardId() const { return shard_id_; }

  inline void setShardId(uint32_t shard_id) { shard_id_ = shard_id; }

  inline void setRole(const DBRole& role) { role_ = role; }

  inline const DBRole& getRole() const { return role_; }

  inline void setDc(const std::string& dc) { dc_ = dc; }

  inline const std::string& getDc() const { return dc_; }

  inline int64_t getPartitionHash() const {
    int64_t key = CityHash64WithSeed(database_name_.data(), database_name_.size(), partition_id_);
    return CityHash64WithSeed(table_name_.data(), table_name_.size(), key);
  }

  inline std::string describe() const {
    std::stringstream ss;
    ss << "Partition{"
       << "DatabaseName='" << database_name_ << "'"
       << ", "
       << "TableName=" << table_name_ << ", "
       << "SrcShardId=" << src_shard_id_ << ", "
       << "ShardId=" << shard_id_ << ", "
       << "Role=" << role_ << ", "
       << "Hash=" << getPartitionHash() << ", "
       << "Dc=" << getDc() << ", "
       << "PartitionId=" << partition_id_ << "}";
    return ss.str();
  }

 private:
  std::string database_name_;
  std::string table_name_;
  uint32_t partition_id_{0};
  uint32_t src_shard_id_{0};
  uint32_t shard_id_{0};
  DBRole role_{DBRole::LEADER};
  std::string dc_{service_router::DEFAULT_DC};
};

inline std::ostream& operator<<(std::ostream& os, const Partition& value) {
  os << value.describe();
  return os;
}

struct PartitionCompare {
  bool operator()(const std::shared_ptr<Partition>& one, const std::shared_ptr<Partition>& two) const {
    if (one->getPartitionHash() == two->getPartitionHash()) {  // 相等必须返回 false, 容器对象通过 one 作为左值 two
      // 操作符比较返回 false, two 作为左值 onw 作为操作数返回
      // false 作为 one == two 相等的依据
      return false;
    }

    return one->getPartitionHash() > two->getPartitionHash();
  }
};

using PartitionPtrSet = std::set<std::shared_ptr<Partition>, PartitionCompare>;
using NotifyPartitionUpdate = folly::Function<void(const PartitionPtrSet&, const PartitionPtrSet&)>;

class PartitionManager {
 public:
  PartitionManager(std::shared_ptr<ConfigManager> config, const std::string& group_name, uint32_t node_id,
                   const std::string& node_dc);
  virtual ~PartitionManager() = default;

  inline static folly::Optional<uint32_t> getPartitionId(const std::shared_ptr<ConfigManager>& config,
                                                         const std::string& database_name,
                                                         const std::string& table_name,
                                                         const laser::LaserKeyFormat& format_key) {
    auto table_schema = config->getTableSchema(database_name, table_name);
    if (!table_schema) {
      return folly::none;
    }

    return getPartitionId(database_name, table_name, format_key, table_schema.value()->getPartitionNumber());
  }

  inline static uint32_t getPartitionId(const std::string& database_name, const std::string& table_name,
                                        const laser::LaserKeyFormat& format_key, uint32_t partition_number) {
    int64_t key = CityHash64WithSeed(database_name.data(), database_name.size(), format_key.getKeyHash());
    key = CityHash64WithSeed(table_name.data(), table_name.size(), key);

    return std::abs(key) % partition_number;
  }

  inline static folly::Optional<uint32_t> getShardId(const std::shared_ptr<Partition>& partition,
                                                     const std::shared_ptr<ConfigManager>& config,
                                                     const std::string& dc) {
    auto shard_number = config->getShardNumber(dc);
    if (!shard_number || shard_number.value() == 0) {
      return folly::none;
    }
    return std::abs(partition->getPartitionHash()) % shard_number.value();
  }

  virtual void updateShardList(const NodeShardList& list,
                               const std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>& nodes);

  virtual inline void subscribe(NotifyPartitionUpdate notify) { partition_update_notify_ = std::move(notify); }

  virtual inline const std::vector<uint32_t> getLeaderShardList() {
    return leader_shard_list_.withRLock([this](auto& list) { return list; });
  }

  virtual inline const std::vector<uint32_t> getFollowerShardList() {
    return follower_shard_list_.withRLock([this](auto& list) { return list; });
  }

  virtual inline std::shared_ptr<std::vector<int64_t>> getPartitionHashList() {
    std::shared_ptr<std::vector<int64_t>> partition_hash_list = std::make_shared<std::vector<int64_t>>();
    partitions_.withRLock([this, &partition_hash_list](auto& partitions) {
      for (auto& partition : partitions) {
        partition_hash_list->emplace_back(partition->getPartitionHash());
      }
    });
    return partition_hash_list;
  }

 private:
  std::shared_ptr<ConfigManager> config_;
  std::string group_name_;
  uint32_t node_id_;
  std::string node_dc_;
  folly::Synchronized<std::vector<uint32_t>> leader_shard_list_;
  folly::Synchronized<std::vector<uint32_t>> follower_shard_list_;
  folly::Synchronized<PartitionPtrSet> partitions_;
  NotifyPartitionUpdate partition_update_notify_;
};

}  // namespace laser
