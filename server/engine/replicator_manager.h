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

#pragma once

#include "folly/executors/CPUThreadPoolExecutor.h"

#include "common/service_router/thrift.h"

#include "replicator_service.h"

namespace laser {

class ReplicatorManager : public std::enable_shared_from_this<ReplicatorManager> {
 public:
  ReplicatorManager();
  virtual ~ReplicatorManager();
  virtual void init(const std::string& service_name, const std::string& host, uint32_t port, int64_t node_hash);
  virtual void addDB(const DBRole& role, uint32_t shard_id, int64_t db_hash, const std::string& version,
                     std::shared_ptr<WdtReplicatorManager> wdt_manager, std::weak_ptr<ReplicationDB> db);
  virtual void removeDB(int64_t db_hash);
  virtual folly::Optional<std::weak_ptr<ReplicationDB>> getDB(int64_t db_hash);
  virtual void setShardList(const std::vector<uint32_t>& leader_shard_list,
                            const std::vector<uint32_t>& follower_shard_list);

 private:
  folly::Synchronized<std::unordered_map<int64_t, std::weak_ptr<ReplicationDB>>> dbs_;
  std::shared_ptr<folly::IOThreadPoolExecutor> replicate_thread_pool_;
  std::thread thrift_server_thread_;
  std::shared_ptr<laser::ReplicatorService> handler_;
  service_router::Server api_server_;
  std::atomic<bool> has_api_server_{false};
  std::string service_name_;
  int64_t node_hash_;
  folly::Synchronized<std::vector<uint32_t>> leader_shard_list_;
  folly::Synchronized<std::vector<uint32_t>> follower_shard_list_;
  folly::SaturatingSemaphore<true> wait_api_server_start_;
};

}  // namespace laser
