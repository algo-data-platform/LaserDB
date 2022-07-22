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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#pragma once

#include "common/laser/if/gen-cpp2/LaserService.h"
#include "common/metrics/metrics.h"
#include "common/laser/config_manager.h"
#include "common/laser/format.h"
#include "common/laser/partition.h"

#include "engine/rocksdb.h"
#include "database_manager.h"

namespace laser {

using LaserServiceCallbackFunc = folly::Function<void(std::shared_ptr<RocksDbEngine>, std::shared_ptr<LaserKeyFormat>)>;
struct DispatchRequestItem {
  std::shared_ptr<LaserKeyFormat> key;
  uint32_t index;
  bool deny_by_traffic_restriction = false;
};
using LaserServiceMultiDispatch =
    folly::Function<void(const std::unordered_map<RocksDbEngine*, std::vector<DispatchRequestItem>>&)>;

class LaserService : virtual public LaserServiceSvIf {
 public:
  LaserService(std::shared_ptr<ConfigManager> config_manager, std::shared_ptr<DatabaseManager> database_manager);
  virtual ~LaserService() = default;

  void delkey(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void expire(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t time) override;
  void expireAt(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t time_at) override;
  void ttl(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  // raw string
  void get(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void append(LaserResponse& result, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) override;
  void sset(LaserResponse& response, std::unique_ptr<LaserKV> kv) override;
  void setx(LaserResponse& response, std::unique_ptr<LaserKV> kv, std::unique_ptr<LaserSetOption> option) override;
  void mget(LaserResponse& response, std::unique_ptr<LaserKeys> keys) override;
  void mgetDetail(LaserResponse& response, std::unique_ptr<LaserKeys> keys) override;
  void mset(LaserResponse& response, std::unique_ptr<LaserKVs> values) override;
  void msetDetail(LaserResponse& response, std::unique_ptr<LaserKVs> values,
                  std::unique_ptr<LaserSetOption> option) override;
  void mdel(LaserResponse& response, std::unique_ptr<LaserKeys> keys) override;
  void exist(LaserResponse& response, std::unique_ptr<LaserKey> key) override;

  // map
  void hget(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) override;
  void hgetall(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void hexists(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) override;
  void hkeys(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void hlen(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void hmget(LaserResponse& response, std::unique_ptr<LaserKey> key,
             std::unique_ptr<std::vector<std::string>> fields) override;
  void hset(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field,
            std::unique_ptr<std::string> value) override;
  void hmset(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<LaserValue> values) override;
  void hdel(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) override;

  // counter
  void decr(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void incr(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void decrBy(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t step) override;
  void incrBy(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t step) override;
  // list
  void lindex(LaserResponse& response, std::unique_ptr<LaserKey> key, int32_t index) override;
  void llen(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void lpop(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void lpush(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) override;
  void lrange(LaserResponse& response, std::unique_ptr<LaserKey> key, int32_t start, int32_t end) override;
  void rpop(LaserResponse& response, std::unique_ptr<LaserKey> key) override;
  void rpush(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) override;
  // set
  void sadd(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) override;
  void sismember(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) override;
  void sremove(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) override;

  void smembers(LaserResponse& response, std::unique_ptr<LaserKey> key) override;

  // zset
  void zadd(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<LaserValue> member_scores) override;
  void zrangeByScore(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t min, int64_t max) override;
  void zremRangeByScore(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t min, int64_t max) override;

 private:
  std::shared_ptr<metrics::Timers> laser_service_timers_;
  std::shared_ptr<ConfigManager> config_manager_;
  std::shared_ptr<DatabaseManager> database_manager_;
  std::shared_ptr<TableTrafficRestrictionMap> traffic_restriction_config_;
  std::shared_ptr<TableTrafficRestrictionMap> second_traffic_restriction_config_;

  void commonCallEngine(std::unique_ptr<LaserKey> key, LaserServiceCallbackFunc func, const std::string& command_name);
  void dispatchRequest(const std::vector<LaserKey>& keys, LaserServiceMultiDispatch func,
                       const std::string& command_name);
  void updateTrafficRestrictionConfig(const TableTrafficRestrictionMap& traffic_restrictions);

  virtual void getDatabaseEngine(std::shared_ptr<RocksDbEngine>* db, const std::unique_ptr<LaserKey>& key,
                                 std::shared_ptr<LaserKeyFormat> format_key);
  virtual std::shared_ptr<LaserKeyFormat> getFormatKey(const std::unique_ptr<LaserKey>& key);
};

}  // namespace laser
