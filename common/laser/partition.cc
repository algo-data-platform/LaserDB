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

#include "partition.h"
#include "common/service_router/router.h"

namespace laser {

PartitionManager::PartitionManager(std::shared_ptr<ConfigManager> config, const std::string& group_name,
                                   uint32_t node_id, const std::string& node_dc)
    : config_(config), group_name_(group_name), node_id_(node_id), node_dc_(node_dc) {
  config_->subscribe(group_name_, node_id_,
                     std::bind(&PartitionManager::updateShardList, this, std::placeholders::_1, std::placeholders::_2));
}

void PartitionManager::updateShardList(const NodeShardList& list,
                                       const std::unordered_map<uint64_t, std::shared_ptr<TableSchema>>& tables) {
  PartitionPtrSet new_partitions;
  std::vector<uint32_t> leader_shard_list = list.getLeaderShardList();
  leader_shard_list_.withWLock([&leader_shard_list, this](auto& list) { list = leader_shard_list; });
  std::vector<uint32_t> follower_shard_list = list.getFollowerShardList();
  follower_shard_list_.withWLock([&follower_shard_list, this](auto& list) { list = follower_shard_list; });

  for (auto& table : tables) {
    if (node_dc_ != table.second->getDc() && node_dc_ != table.second->getDistDc()) {
      LOG(INFO) << "dont't care this table:" << table.second->getTableName();
      continue;
    }
    if (list.getIsEdgeNode()) {
      std::string group_node_id = folly::to<std::string>(group_name_, "#", node_id_);
      auto bind_edge_nodes = table.second->getBindEdgeNodes();
      if (std::find(bind_edge_nodes.begin(), bind_edge_nodes.end(), group_node_id) == bind_edge_nodes.end()) {
        continue;
      }
    }
    uint32_t partition_number = table.second->getPartitionNumber();
    bool is_migrate = (node_dc_ == table.second->getDistDc() && node_dc_ != table.second->getDc());
    for (int i = 0; i < partition_number; i++) {
      std::shared_ptr<Partition> new_partition =
          std::make_shared<Partition>(table.second->getDatabaseName(), table.second->getTableName(), i);
      auto src_shard_id = getShardId(new_partition, config_, table.second->getDc());
      if (!src_shard_id) {
        LOG(INFO) << "Get src shard id fail, table: " << *(table.second);
        continue;
      }
      new_partition->setSrcShardId(src_shard_id.value());
      new_partition->setDc(table.second->getDc());
      uint32_t shard_id = src_shard_id.value();
      if (is_migrate) {
        auto dst_shard_id = getShardId(new_partition, config_, table.second->getDistDc());
        if (!dst_shard_id) {
          LOG(INFO) << "Get dist shard id fail, table: " << *(table.second);
          continue;
        }
        new_partition->setShardId(dst_shard_id.value());
        shard_id = dst_shard_id.value();
      } else {
        new_partition->setShardId(src_shard_id.value());
      }
      if (std::find(leader_shard_list.begin(), leader_shard_list.end(), shard_id) != leader_shard_list.end()) {
        if (is_migrate) {
          new_partition->setRole(DBRole::FOLLOWER);
        } else {
          new_partition->setRole(DBRole::LEADER);
        }
        new_partitions.insert(new_partition);
      }
      if (std::find(follower_shard_list.begin(), follower_shard_list.end(), shard_id) != follower_shard_list.end()) {
        new_partition->setRole(DBRole::FOLLOWER);
        new_partitions.insert(new_partition);
      }
    }
  }

  PartitionPtrSet mount_partitions;
  PartitionPtrSet unmount_partitions;
  partitions_.withWLock([&new_partitions, &mount_partitions, &unmount_partitions, this](auto& partitions) {
    std::set_difference(partitions.begin(), partitions.end(), new_partitions.begin(), new_partitions.end(),
                        std::inserter(unmount_partitions, unmount_partitions.begin()),
                        [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });
    std::set_difference(new_partitions.begin(), new_partitions.end(), partitions.begin(), partitions.end(),
                        std::inserter(mount_partitions, mount_partitions.begin()),
                        [](auto& t1, auto& t2) { return laser::PartitionCompare()(t1, t2); });

    // 检查 role 是否变更
    std::unordered_map<int64_t, DBRole> partition_roles;
    std::unordered_map<int64_t, std::string> partition_dcs;
    for (auto& partition : partitions) {
      partition_roles[partition->getPartitionHash()] = partition->getRole();
      partition_dcs[partition->getPartitionHash()] = partition->getDc();
    }
    for (auto& partition : new_partitions) {
      int64_t hash = partition->getPartitionHash();
      if (partition_roles.find(hash) != partition_roles.end()) {
        if (partition_roles[hash] != partition->getRole()) {
          VLOG(5) << "Partition:" << partition << " role has changed";
          mount_partitions.insert(partition);
        } else if (partition_dcs[hash] != partition->getDc()) {
          LOG(INFO) << "Partition:" << partition << " dc has changed";
          mount_partitions.insert(partition);
        }
      }
    }
    partitions = new_partitions;
  });

  if (partition_update_notify_) {
    partition_update_notify_(mount_partitions, unmount_partitions);
  }
}

}  // namespace laser
