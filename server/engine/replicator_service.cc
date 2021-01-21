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

#include "common/laser/status.h"

#include "replicator_service.h"
#include "replicator_manager.h"

namespace laser {

ReplicatorService::ReplicatorService(std::weak_ptr<ReplicatorManager> replicator_manager)
    : replicator_manager_(replicator_manager) {}

void ReplicatorService::async_tm_replicate(
    std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateResponse>>> callback,
    std::unique_ptr<::laser::ReplicateRequest> request) {
  VLOG(5) << "request, db_hash:" << request->db_hash;
  auto db = getDB(request->db_hash);
  if (!db) {
    LaserException ex = createLaserException(Status::RP_SOURCE_NOT_FOUND,
                                             folly::to<std::string>("could not find db ", request->db_hash));
    callback->exception(ex);
    return;
  }

  db.value()->handleReplicateRequest(std::move(callback), std::move(request));
}

void ReplicatorService::async_tm_replicateWdt(
    std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateWdtResponse>>> callback,
    std::unique_ptr<::laser::ReplicateWdtRequest> request) {
  auto db = getDB(request->db_hash);
  if (!db) {
    LaserException ex = createLaserException(Status::RP_SOURCE_NOT_FOUND,
                                             folly::to<std::string>("could not find db ", request->db_hash));
    callback->exception(ex);
    return;
  }

  db.value()->handleReplicateWdtRequest(std::move(callback), std::move(request));
}

folly::Optional<std::shared_ptr<ReplicationDB>> ReplicatorService::getDB(int64_t db_hash) {
  auto manager = replicator_manager_.lock();
  if (!manager) {
    return folly::none;
  }
  auto weak_db = manager->getDB(db_hash);
  if (!weak_db) {
    return folly::none;
  }

  auto db = weak_db.value().lock();
  if (!db) {
    return folly::none;
  }
  return db;
}

}  // namespace laser
