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

#include "partition.h"
#include "common/service_router/router.h"

namespace laser {

PartitionManager::PartitionManager(std::shared_ptr<ConfigManager> config, const std::string& group_name,
                                   uint32_t node_id)
    : config_(config), group_name_(group_name), node_id_(node_id) {
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
    if (list.getIsEdgeNode()) {
      std::string group_node_id = folly::to<std::string>(group_name_, "#", node_id_);
      auto bind_edge_nodes = table.second->getBindEdgeNodes();
      if (std::find(bind_edge_nodes.begin(), bind_edge_nodes.end(), group_node_id) == bind_edge_nodes.end()) {
        continue;
      }
    }
    uint32_t partition_number = table.second->getPartitionNumber();
    for (int i = 0; i < partition_number; i++) {
      std::shared_ptr<Partition> new_partition =
          std::make_shared<Partition>(table.second->getDatabaseName(), table.second->getTableName(), i);
      auto shard_id = getShardId(new_partition, config_);
      if (!shard_id) {
        LOG(INFO) << "Get shard id fail, table: " << *(table.second);
        continue;
      }
      new_partition->setShardId(shard_id.value());
      if (std::find(leader_shard_list.begin(), leader_shard_list.end(), shard_id.value()) != leader_shard_list.end()) {
        new_partition->setRole(DBRole::LEADER);
        new_partitions.insert(new_partition);
      }
      if (std::find(follower_shard_list.begin(), follower_shard_list.end(), shard_id.value()) !=
          follower_shard_list.end()) {
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
    for (auto& partition : partitions) {
      partition_roles[partition->getPartitionHash()] = partition->getRole();
    }
    for (auto& partition : new_partitions) {
      int64_t hash = partition->getPartitionHash();
      if (partition_roles.find(hash) != partition_roles.end() && partition_roles[hash] != partition->getRole()) {
        VLOG(5) << "Partition:" << partition << " role has change.";
        mount_partitions.insert(partition);
      }
    }
    partitions = new_partitions;
  });

  if (partition_update_notify_) {
    partition_update_notify_(mount_partitions, unmount_partitions);
  }
}

}  // namespace laser
