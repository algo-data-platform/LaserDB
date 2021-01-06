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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author Junpeng Liu <liujunpeng555@gmail.com>
 */

#pragma once

#include "common/laser/config_manager.h"
#include "common/service_router/connection_pool.h"
#include "client/laser_client.h"

#include "proxy_config.h"

constexpr static uint32_t REDIS_REQ_VERSION_LOCATION = 0;
constexpr static uint32_t REDIS_REQ_DATABASE_NAME_LOCATION = 1;
constexpr static uint32_t REDIS_REQ_TABLE_NAME_LOCATION = 2;
constexpr static uint32_t REDIS_REQ_PRIMARY_KEY_LOCATION = 3;
constexpr static uint32_t REDIS_REQ_MIN_NUM = 4;
constexpr static char LASER_PROXY_DELIMITER_FIRST[] = "laser-proxy-delimiter-first";
constexpr static char LASER_PROXY_DELIMITER_SECOND[] = "laser-proxy-delimiter-second";
constexpr static char LASER_CLIENT_RECEIVE_TIMEOUT[] = "laser-client-receive-timeout";
constexpr static char LASER_CLIENT_READ_MODE[] = "laser-proxy-read-mode";

namespace laser {
class RedisCommandProcess {
 public:
  explicit RedisCommandProcess(std::shared_ptr<laser::LaserClient> laser_client,
                               std::shared_ptr<laser::ProxyConfig> proxy_config);
  bool getLaserKey(LaserKey* laser_key, const std::string& key);
  bool getLaserKV(LaserKV* laser_kv, const std::string& key, const std::string& value);
  void setKeyReceiveTimeoutMs(const std::vector<std::string>& keys);
  void setKVReceiveTimeoutMs(const std::map<std::string, std::string>& kvs);
  bool configGet(std::string* res, const std::string& key);
  bool configSet(const std::string& key, const std::string& value);
  bool get(std::string* res, const std::string& key);
  bool set(const std::string& key, const std::string& value);
  bool setex(const std::string& key, int64_t milliseconds, const std::string& value);
  bool append(uint32_t* length, const std::string& key, const std::string& value);
  bool exists(uint32_t* res, const std::vector<std::string>& keys);
  bool mget(std::vector<std::string>* res, const std::vector<std::string>& keys);
  bool mset(std::vector<std::string>* res, const std::map<std::string, std::string>& kvs);
  bool hget(std::string* res, const std::string& key, const std::string& field);
  bool hset(const std::string& key, const std::string& field, const std::string& value);
  bool hmget(std::vector<std::string>* res, const std::string& key, const std::vector<std::string>& fields);
  bool hmset(const std::string& key, const std::map<std::string, std::string>& fvs);
  bool hgetall(std::map<std::string, std::string>* res, const std::string& key);
  bool hkeys(std::vector<std::string>* res, const std::string& key);
  bool hlen(uint32_t* res, const std::string& key);
  bool hexists(const std::string& key, const std::string& field);
  bool hdel(const std::string& key, const std::string& field);
  bool del(uint32_t* res, const std::vector<std::string>& keys);
  bool expire(const std::string& key, int64_t time);
  bool expireat(const std::string& key, int64_t time_at);
  bool ttl(int64_t* res, const std::string& key);
  bool decr(int64_t* res, const std::string& key);
  bool incr(int64_t* res, const std::string& key);
  bool decrby(int64_t* res, const std::string& key, int64_t step);
  bool incrby(int64_t* res, const std::string& key, int64_t step);
  bool zadd(uint32_t* res, const std::string& key, const std::unordered_map<std::string, double>& member_scores);
  bool zrangebyscore(std::vector<LaserFloatScoreMember>* res, const std::string& key, double min, double max);
  bool zremrangebyscore(uint32_t* res, const std::string& key, double min, double max);

  enum class Option {
    DELIMITER_FIRST = 0,
    DELIMITER_SECOND,
    RECEIVE_TIMEOUT,
    READ_MODE,
  };

  enum class Operation {
    OPERATE_READ = 0,
    OPERATE_WRITE,
  };

  void setLaserClientOptionReceiveTimeout(laser::LaserKey laser_key, Operation operate);

 private:
  std::shared_ptr<laser::LaserClient> laser_client_;
  std::shared_ptr<laser::ProxyConfig> proxy_config_;
  std::map<const std::string, Option> option_table_;
  laser::ClientOption laser_client_option_;
  char laser_proxy_redis_delimiter_first_;
  char laser_proxy_redis_delimiter_second_;
  std::shared_ptr<metrics::Histograms> mget_total_key_his_;
  std::shared_ptr<metrics::Histograms> mget_null_value_his_;
  std::shared_ptr<metrics::Timers> hget_command_timers_;
  std::shared_ptr<metrics::Timers> hset_command_timers_;
  std::shared_ptr<metrics::Timers> hmget_command_timers_;
  std::shared_ptr<metrics::Timers> zadd_command_timers_;
  std::shared_ptr<metrics::Timers> zrangebyscore_command_timers_;
  std::shared_ptr<metrics::Timers> zremrangebyscore_command_timers_;
  std::shared_ptr<metrics::Timers> hmset_command_timers_;
  std::shared_ptr<metrics::Timers> hgetall_command_timers_;
  std::shared_ptr<metrics::Timers> hkeys_command_timers_;
  std::shared_ptr<metrics::Timers> hlen_command_timers_;
  std::shared_ptr<metrics::Timers> hexists_command_timers_;
  std::shared_ptr<metrics::Timers> hdel_command_timers_;
  std::shared_ptr<metrics::Histograms> hmget_total_key_his_;
  std::shared_ptr<metrics::Histograms> hmget_null_value_his_;
  std::shared_ptr<metrics::Timers> del_command_timers_;
  std::shared_ptr<metrics::Timers> expire_command_timers_;
  std::shared_ptr<metrics::Timers> expireat_command_timers_;
  std::shared_ptr<metrics::Timers> ttl_command_timers_;
  std::shared_ptr<metrics::Timers> decr_command_timers_;
  std::shared_ptr<metrics::Timers> incr_command_timers_;
  std::shared_ptr<metrics::Timers> decrby_command_timers_;
  std::shared_ptr<metrics::Timers> incrby_command_timers_;
  std::shared_ptr<metrics::Timers> exists_command_timers_;
};

}  // namespace laser
