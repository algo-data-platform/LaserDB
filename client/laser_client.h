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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#pragma once

#include <cstdint>
#include "folly/executors/CPUThreadPoolExecutor.h"

#include "common/laser/if/gen-cpp2/LaserService.h"
#include "common/laser/config_manager.h"
#include "common/laser/partition.h"
#include "common/service_router/router.h"
#include "common/service_router/thrift.h"

#include "options.h"

namespace laser {

constexpr static uint32_t LASER_FLOAT_AMPLIFICATION_FACTOR = 10000;

class FutureTimeoutException : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

class ServerRelationKeys {
 public:
  explicit ServerRelationKeys(std::shared_ptr<service_router::ServerAddress> address);
  ~ServerRelationKeys() = default;
  void addIndex(unsigned int index);

  inline std::shared_ptr<service_router::ServerAddress> getAddress() { return address_ptr_; }

  inline const std::vector<uint32_t>& getIndexes() { return indexes_; }

 private:
  std::shared_ptr<service_router::ServerAddress> address_ptr_;
  std::vector<unsigned int> indexes_;
};

// Laser client 每次需要初始化一个 ConfigManager 实例，需要和 consul 进行同步
// 因此不建议频繁创建 client 实例，一般一个进程使用一个实例即可
using FutureResponse = folly::Future<::laser::LaserResponse>;
using ThriftProcessRequestFunc =
    folly::Function<void(std::unique_ptr<laser::LaserServiceAsyncClient>, apache::thrift::RpcOptions&) const>;
using ThriftSendRequestFunc = folly::Function<folly::Optional<std::shared_ptr<FutureResponse>>()>;
using ThriftResponseProcessFunc = folly::Function<Status(const LaserResponse&, const Status&)>;
using ThriftTryResponseProcessFunc = folly::Function<Status(folly::Try<laser::LaserResponse>&)>;
using GetResponseProcessFunc = folly::Function<void(folly::Try<::laser::LaserResponse>&)>;
using CollectAllResponseProcessFunc = folly::Function<void(const std::vector<folly::Try<LaserResponse>>&)>;

class LaserFloatScoreMember {
 public:
  inline const std::string& getMember() const { return member_; }

  inline void setMember(const std::string& member) { member_ = member; }

  inline double getScore() const { return score_; }

  inline void setScore(double score) { score_ = score; }

 private:
  std::string member_;
  double score_;
};

class LaserClientResource {
 public:
  static std::shared_ptr<LaserClientResource> getInstance();
  LaserClientResource();
  virtual ~LaserClientResource();
  std::shared_ptr<laser::ConfigManager> getOrCreateConfigManager(const std::string& service_name);
  inline std::shared_ptr<folly::CPUThreadPoolExecutor> getWorkPool() { return work_thread_pool_; }

 private:
  folly::Synchronized<std::unordered_map<std::string, std::shared_ptr<laser::ConfigManager>>> config_managers_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> work_thread_pool_;
};

class LaserClient {
 public:
  explicit LaserClient(const std::string& target_service_name) : target_service_name_(target_service_name) {}
  ~LaserClient() = default;

  void init();

  folly::Optional<std::shared_ptr<FutureResponse>> del(const ClientOption& options, const LaserKey& key);
  Status delSync(const ClientOption& options, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> expire(const ClientOption& options, const LaserKey& key,
                                                          int64_t time);
  Status expireSync(const ClientOption& options, const LaserKey& key, int64_t time);
  folly::Optional<std::shared_ptr<FutureResponse>> expireAt(const ClientOption& options, const LaserKey& key,
                                                            int64_t time_at);
  Status expireAtSync(const ClientOption& options, const LaserKey& key, int64_t time_at);
  folly::Optional<std::shared_ptr<FutureResponse>> ttl(const ClientOption& options, const LaserKey& key);
  Status ttlSync(const ClientOption& options, int64_t* result, const LaserKey& key);

  folly::Optional<std::shared_ptr<FutureResponse>> get(const ClientOption& options, const LaserKey& key);
  Status getSync(const ClientOption& options, std::string* data, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> set(const ClientOption& options, const LaserKV& kv);
  Status setSync(const ClientOption& options, const LaserKV& kv);
  folly::Optional<std::shared_ptr<FutureResponse>> append(const ClientOption& options, const LaserKey& key,
                                                          const std::string& value);
  Status appendSync(const ClientOption& options, uint32_t* length, const LaserKey& key, const std::string& value);
  folly::Optional<std::shared_ptr<FutureResponse>> setx(const ClientOption& options, const LaserKV& kv,
                                                        const LaserSetOption& set_option);
  Status setxSync(const ClientOption& options, const LaserKV& kv, const LaserSetOption& set_option);
  Status mget(const ClientOption& options, std::vector<LaserValue>* values, const std::vector<LaserKey>& keys);
  Status mgetDetail(const ClientOption& options, std::vector<LaserValue>* values, const std::vector<LaserKey>& keys);
  Status mset(const ClientOption& options, std::vector<int64_t>* result, const std::vector<LaserKV>& kvs);
  Status msetDetail(const ClientOption& options, std::vector<LaserValue>* values, const std::vector<LaserKV>& kvs);
  Status msetDetail(const ClientOption& options, std::vector<LaserValue>* values, const std::vector<LaserKV>& kvs,
                    const LaserSetOption& set_option);
  Status mdel(const ClientOption& options, std::vector<LaserValue>* values, const std::vector<LaserKey>& keys);
  folly::Optional<std::shared_ptr<FutureResponse>> exist(const ClientOption& options, const LaserKey& key);
  Status existSync(const ClientOption& options, bool* data, const LaserKey& key);

  // map
  folly::Optional<std::shared_ptr<FutureResponse>> hset(const ClientOption& options, const LaserKey& key,
                                                        const std::string& field, const std::string& value);
  Status hsetSync(const ClientOption& options, const LaserKey& key, const std::string& field, const std::string& value);
  folly::Optional<std::shared_ptr<FutureResponse>> hget(const ClientOption& options, const LaserKey& key,
                                                        const std::string& field);
  Status hgetSync(const ClientOption& options, std::string* data, const LaserKey& key, const std::string& field);
  folly::Optional<std::shared_ptr<FutureResponse>> hdel(const ClientOption& options, const LaserKey& key,
                                                        const std::string& field);
  Status hdelSync(const ClientOption& options, const LaserKey& key, const std::string& field);
  folly::Optional<std::shared_ptr<FutureResponse>> hexists(const ClientOption& options, const LaserKey& key,
                                                           const std::string& field);
  Status hexistsSync(const ClientOption& options, const LaserKey& key, const std::string& field);
  folly::Optional<std::shared_ptr<FutureResponse>> hgetall(const ClientOption& options, const LaserKey& key);
  Status hgetallSync(const ClientOption& options, std::map<std::string, std::string>* data, const LaserKey& key);

  folly::Optional<std::shared_ptr<FutureResponse>> hlen(const ClientOption& options, const LaserKey& key);
  Status hlenSync(const ClientOption& options, uint32_t* len, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> hmget(const ClientOption& options, const LaserKey& key,
                                                         const std::vector<std::string>& fields);
  Status hmgetSync(const ClientOption& options, std::map<std::string, std::string>* data, const LaserKey& key,
                   const std::vector<std::string>& fields);
  folly::Optional<std::shared_ptr<FutureResponse>> hmset(const ClientOption& options, const LaserKey& key,
                                                         const LaserValue& values);
  Status hmsetSync(const ClientOption& options, const LaserKey& key, const LaserValue& values);
  folly::Optional<std::shared_ptr<FutureResponse>> hkeys(const ClientOption& options, const LaserKey& key);
  Status hkeysSync(const ClientOption& options, std::vector<std::string>* data, const LaserKey& key);

  // list
  folly::Optional<std::shared_ptr<FutureResponse>> lindex(const ClientOption& options, const LaserKey& key,
                                                          uint32_t index);
  Status lindexSync(const ClientOption& options, std::string* item, const LaserKey& key, uint32_t index);
  folly::Optional<std::shared_ptr<FutureResponse>> llen(const ClientOption& options, const LaserKey& key);
  Status llenSync(const ClientOption& options, uint32_t* len, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> lpop(const ClientOption& options, const LaserKey& key);
  Status lpopSync(const ClientOption& options, std::string* item, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> rpop(const ClientOption& options, const LaserKey& key);
  Status rpopSync(const ClientOption& options, std::string* item, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> lpush(const ClientOption& options, const LaserKey& key,
                                                         const std::string& value);
  Status lpushSync(const ClientOption& options, const LaserKey& key, const std::string& value);
  folly::Optional<std::shared_ptr<FutureResponse>> rpush(const ClientOption& options, const LaserKey& key,
                                                         const std::string& value);
  Status rpushSync(const ClientOption& options, const LaserKey& key, const std::string& value);
  folly::Optional<std::shared_ptr<FutureResponse>> lrange(const ClientOption& options, const LaserKey& key,
                                                          uint32_t start, uint32_t end);
  Status lrangeSync(const ClientOption& options, std::vector<std::string>* list, const LaserKey& key, uint32_t start,
                    uint32_t end);

  // counter
  folly::Optional<std::shared_ptr<FutureResponse>> decr(const ClientOption& options, const LaserKey& key);
  Status decrSync(const ClientOption& options, int64_t* result, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> incr(const ClientOption& options, const LaserKey& key);
  Status incrSync(const ClientOption& options, int64_t* result, const LaserKey& key);
  folly::Optional<std::shared_ptr<FutureResponse>> decrBy(const ClientOption& options, const LaserKey& key,
                                                          int64_t step);
  Status decrBySync(const ClientOption& options, int64_t* result, const LaserKey& key, int64_t step);
  folly::Optional<std::shared_ptr<FutureResponse>> incrBy(const ClientOption& options, const LaserKey& key,
                                                          int64_t step);
  Status incrBySync(const ClientOption& options, int64_t* result, const LaserKey& key, int64_t step);

  // zset
  folly::Optional<std::shared_ptr<FutureResponse>> zadd(const ClientOption& options, const LaserKey& key,
                                                        const std::unordered_map<std::string, double>& member_scores);
  Status zaddSync(const ClientOption& options, uint32_t* res, const LaserKey& key,
                  const std::unordered_map<std::string, double>& member_scores);
  folly::Optional<std::shared_ptr<FutureResponse>> zrangebyscore(const ClientOption& options, const LaserKey& key,
                                                                 double min, double max);
  Status zrangebyscoreSync(const ClientOption& options, std::vector<LaserFloatScoreMember>* res, const LaserKey& key,
                           double min, double max);
  folly::Optional<std::shared_ptr<FutureResponse>> zremrangebyscore(const ClientOption& options, const LaserKey& key,
                                                                    double min, double max);
  Status zremrangebyscoreSync(const ClientOption& options, uint32_t* res, const LaserKey& key, double min, double max);

  // future process
  void getResult(std::shared_ptr<FutureResponse> response, GetResponseProcessFunc process_func);
  void collectAllResult(const std::vector<std::shared_ptr<FutureResponse>>& responses,
                        CollectAllResponseProcessFunc process_func);
  Status getProcess(std::string* data, const folly::Try<LaserResponse>& t);
  Status boolProcess(bool* data, const folly::Try<LaserResponse>& t);
  Status mapProcess(std::map<std::string, std::string>* data, const folly::Try<LaserResponse>& t);
  Status listProcess(std::vector<std::string>* data, const folly::Try<LaserResponse>& t);
  Status scoreMemberProcess(std::vector<LaserFloatScoreMember>* data, const folly::Try<LaserResponse>& t);
  Status intProcess(uint32_t* data, const folly::Try<LaserResponse>& t);
  Status int64Process(int64_t* data, const folly::Try<LaserResponse>& t);
  Status setProcess(const folly::Try<LaserResponse>& t);
  Status okProcess(const folly::Try<LaserResponse>& t);

 private:
  std::string target_service_name_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> work_thread_pool_;
  std::shared_ptr<laser::ConfigManager> config_manager_;
  std::shared_ptr<metrics::Meter> call_server_timers_;
  std::shared_ptr<metrics::Meter> call_server_status_ok_;
  std::shared_ptr<metrics::Timers> del_command_timers_;
  std::shared_ptr<metrics::Timers> expire_command_timers_;
  std::shared_ptr<metrics::Timers> expireat_command_timers_;
  std::shared_ptr<metrics::Timers> ttl_command_timers_;
  std::shared_ptr<metrics::Timers> get_command_timers_;
  std::shared_ptr<metrics::Timers> set_command_timers_;
  std::shared_ptr<metrics::Timers> append_command_timers_;
  std::shared_ptr<metrics::Timers> setx_command_timers_;
  std::shared_ptr<metrics::Timers> mget_command_timers_;
  std::shared_ptr<metrics::Timers> mget_detail_command_timers_;
  std::shared_ptr<metrics::Timers> mset_command_timers_;
  std::shared_ptr<metrics::Timers> mset_detail_command_timers_;
  std::shared_ptr<metrics::Timers> mdel_command_timers_;
  std::shared_ptr<metrics::Timers> zadd_command_timers_;
  std::shared_ptr<metrics::Timers> zrangebyscore_command_timers_;
  std::shared_ptr<metrics::Timers> zremrangebyscore_command_timers_;
  std::shared_ptr<metrics::Timers> exist_command_timers_;
  std::shared_ptr<metrics::Timers> hset_command_timers_;
  std::shared_ptr<metrics::Timers> hget_command_timers_;
  std::shared_ptr<metrics::Timers> hdel_command_timers_;
  std::shared_ptr<metrics::Timers> hexists_command_timers_;
  std::shared_ptr<metrics::Timers> hgetall_command_timers_;
  std::shared_ptr<metrics::Timers> hkeys_command_timers_;
  std::shared_ptr<metrics::Timers> hlen_command_timers_;
  std::shared_ptr<metrics::Timers> hmset_command_timers_;
  std::shared_ptr<metrics::Timers> hmget_command_timers_;
  std::shared_ptr<metrics::Timers> llen_command_timers_;
  std::shared_ptr<metrics::Timers> lpop_command_timers_;
  std::shared_ptr<metrics::Timers> lpush_command_timers_;
  std::shared_ptr<metrics::Timers> rpop_command_timers_;
  std::shared_ptr<metrics::Timers> rpush_command_timers_;
  std::shared_ptr<metrics::Timers> lrange_command_timers_;
  std::shared_ptr<metrics::Timers> lindex_command_timers_;
  std::shared_ptr<metrics::Timers> decr_command_timers_;
  std::shared_ptr<metrics::Timers> incr_command_timers_;
  std::shared_ptr<metrics::Timers> decrby_command_timers_;
  std::shared_ptr<metrics::Timers> incrby_command_timers_;

  bool getRouteInfo(uint32_t* shard_id, int64_t* partition_hash, bool* route_to_edge_node, const LaserKey& key,
                    const ClientOption& options);
  bool getRouteInfos(std::vector<std::tuple<uint32_t, int64_t, bool>>* route_infos, const std::vector<LaserKey>& keys,
                     const ClientOption& options);
  std::shared_ptr<service_router::ClientOption> getClientOption(const ClientOption& options,
                                                                uint32_t shard_id = UINT32_MAX,
                                                                int64_t partition_hash = 0,
                                                                bool route_to_edge_node = false);
  void getRetryOption(service_router::ThriftRetryOption* retry_option, const ClientOption& options);
  bool commonCall(const LaserKey& laser_key, const ClientOption& options, ThriftProcessRequestFunc callback);
  bool callThriftServer(uint32_t shard_id, int64_t partition_hash, bool route_to_edge_node, const ClientOption& options,
                        ThriftProcessRequestFunc callback);
  bool callThriftServer(std::shared_ptr<service_router::ServerAddress> address, const ClientOption& options,
                        ThriftProcessRequestFunc callback);
  Status processSync(ThriftSendRequestFunc send_request, ThriftTryResponseProcessFunc process_func, uint32_t timeout);
  Status commonProcess(const folly::Try<LaserResponse>& t, ThriftResponseProcessFunc func, bool only_ok_call);

  void mutilCallDispatchRequest(std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>>* addresses,
                                const ClientOption& options, const std::vector<LaserKey>& keys);
  void getIntMinMax(int64_t* int_min, int64_t* int_max, double double_min, double double_max);
};

}  // namespace laser
