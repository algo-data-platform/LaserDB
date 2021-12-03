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

#include "common/service_router/router.h"

#include "replicator_manager.h"

namespace laser {

DEFINE_int32(replicator_executor_threads, 16, "The number of replicator executor threads.");

ReplicatorManager::ReplicatorManager() {
  replicate_thread_pool_ = std::make_shared<folly::IOThreadPoolExecutor>(
      FLAGS_replicator_executor_threads, std::make_shared<folly::NamedThreadFactory>("ReplicatorPool"));
}

ReplicatorManager::~ReplicatorManager() {
  if (thrift_server_thread_.joinable()) {
    thrift_server_thread_.join();
  }
  if (replicate_thread_pool_) {
    replicate_thread_pool_->stop();
  }
}

void ReplicatorManager::init(const std::string& service_name, const std::string& host, uint32_t port, int64_t node_hash,
                             const std::string& node_dc) {
  // thrift 初始化
  handler_ = std::make_shared<laser::ReplicatorService>(shared_from_this());
  auto thrift_server_modifier = [](service_router::ThriftServer&) {};
  auto server_on_create = [this, &node_dc](const service_router::Server& server) {
    service_router::Router::getInstance()->setDc(server, node_dc);
    // 注册到 database manager 中用来控制服务状态等信息
    leader_shard_list_.withRLock([&server](auto& list) {
      auto router = service_router::Router::getInstance();
      router->setAvailableShardList(server, list);
    });
    follower_shard_list_.withRLock([&server](auto& list) {
      auto router = service_router::Router::getInstance();
      router->setFollowerAvailableShardList(server, list);
    });
    api_server_ = server;
    has_api_server_ = true;
    wait_api_server_start_.post();
  };
  thrift_server_thread_ =
      std::thread([this, &service_name, &host, port, thrift_server_modifier = std::move(thrift_server_modifier),
                   server_on_create = std::move(server_on_create)]() {
        folly::setThreadName("replicateServerStart");
        service_router::thriftServiceServer<laser::ReplicatorService>(
            service_name, host, port, handler_, thrift_server_modifier, 0, service_router::ServerStatus::AVAILABLE,
            server_on_create);
      });
  service_name_ = service_name;
  node_hash_ = node_hash;
}

void ReplicatorManager::setShardList(const std::vector<uint32_t>& leader_shard_list,
                                     const std::vector<uint32_t>& follower_shard_list) {
  if (has_api_server_) {
    auto router = service_router::Router::getInstance();
    router->setAvailableShardList(api_server_, leader_shard_list);
    router->setFollowerAvailableShardList(api_server_, follower_shard_list);
  }

  leader_shard_list_.withWLock([&leader_shard_list](auto& list) { list = leader_shard_list; });
  follower_shard_list_.withWLock([&follower_shard_list](auto& list) { list = follower_shard_list; });
}

void ReplicatorManager::addDB(const DBRole& role, uint32_t shard_id, int64_t db_hash, const std::string& version,
                              std::shared_ptr<WdtReplicatorManager> wdt_manager, std::weak_ptr<ReplicationDB> db,
                              const std::string& src_dc) {
  wait_api_server_start_.wait();
  dbs_.withWLock([this, db_hash, db, &role, &src_dc, shard_id, &version, wdt_manager](auto& dbs) {
    auto replication_db = db.lock();
    if (!replication_db) {
      return;
    }
    std::string client_address = folly::to<std::string>(api_server_.getHost(), ":", api_server_.getPort());
    replication_db->startReplicator(shard_id, db_hash, service_name_, replicate_thread_pool_, role, version, node_hash_,
                                    client_address, wdt_manager, src_dc);
    dbs[db_hash] = db;
  });
}

void ReplicatorManager::removeDB(int64_t db_hash) {
  dbs_.withWLock([this, db_hash](auto& dbs) {
    if (dbs.find(db_hash) == dbs.end()) {
      return;
    }

    dbs.erase(db_hash);
  });
}

folly::Optional<std::weak_ptr<ReplicationDB>> ReplicatorManager::getDB(int64_t db_hash) {
  return dbs_.withRLock([this, db_hash](auto& dbs) -> folly::Optional<std::weak_ptr<ReplicationDB>> {
    if (dbs.find(db_hash) == dbs.end()) {
      return folly::none;
    }

    return dbs.at(db_hash);
  });
}

}  // namespace laser
