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

#include "common/laser/if/gen-cpp2/Replicator.h"

#include "replication_db.h"

namespace laser {

class ReplicatorManager;
class ReplicatorService : virtual public ReplicatorSvIf {
 public:
  explicit ReplicatorService(std::weak_ptr<ReplicatorManager> replicator_manager);
  virtual ~ReplicatorService() = default;
  virtual void async_tm_replicate(
      std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateResponse>>> callback,
      std::unique_ptr<::laser::ReplicateRequest> request);
  virtual void async_tm_replicateWdt(
      std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::laser::ReplicateWdtResponse>>> callback,
      std::unique_ptr<::laser::ReplicateWdtRequest> request);

 private:
  std::weak_ptr<ReplicatorManager> replicator_manager_;
  folly::Optional<std::shared_ptr<ReplicationDB>> getDB(int64_t db_hash);
};

}  // namespace laser
