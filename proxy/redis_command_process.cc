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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author Junpeng Liu <liujunpeng555@gmail.com>
 */

#include "folly/Conv.h"

#include "common/laser/status.h"

#include "redis.h"
#include "redis_command_process.h"

DECLARE_string(host);
DEFINE_string(laser_client_read_mode, "leader_read", "Laser client read mode, leader_read/follower_read/mixed_read");
DEFINE_int32(redis_key_delimiter_first, 1, "Redis key first delimiter,range:0-127");
DEFINE_int32(redis_key_delimiter_second, 2, "Redis key second delimiter,range:0-127");
DEFINE_int32(laser_client_read_timeout, 10, "Laser client read timeout, unit millisecond(ms)");
DEFINE_int32(laser_client_write_timeout, 10, "Laser client write timeout, unit millisecond(ms)");
DEFINE_int32(laser_client_allowed_flow, 100, "Laser clientallowed flow Threshold(%)");
DEFINE_int32(laser_proxy_diff_range, 256, "Service router local first address diff range");
DEFINE_bool(laser_proxy_local_first, true, "Service router loadbalance use local first strategy or not");
DEFINE_string(laser_database_name, "", "Laser database name, default is null");
DEFINE_string(laser_table_name, "", "Laser table name,default is null");
DEFINE_int32(laser_client_max_conn_pre_server, 0, "max pre conn");

namespace laser {
constexpr char LASER_PROXY_MODULE_NAME[] = "laser_proxy";
constexpr char LASER_PROXY_METRIC_COMMAND_GET_TIMER[] = "get_command";
constexpr char LASER_PROXY_METRIC_COMMAND_SET_TIMER[] = "set_command";
constexpr char LASER_PROXY_METRIC_COMMAND_SETEX_TIMER[] = "setex_command";
constexpr char LASER_PROXY_METRIC_COMMAND_APPEND_TIMER[] = "append_command";
constexpr char LASER_PROXY_METRIC_COMMAND_EXISTS_TIMER[] = "exists_command";
constexpr char LASER_PROXY_METRIC_COMMAND_MSET_TIMER[] = "mset_command";
constexpr char LASER_PROXY_METRIC_COMMAND_MGET_TIMER[] = "mget_command";
constexpr char LASER_PROXY_METRIC_COMMAND_ZADD_TIMER[] = "zadd_command";
constexpr char LASER_PROXY_METRIC_COMMAND_ZRANGEBYSCORE_TIMER[] = "zrangebyscore_command";
constexpr char LASER_PROXY_METRIC_COMMAND_ZREMRANGEBYSCORE_TIMER[] = "zremrangebyscore_command";
constexpr char LASER_PROXY_METRIC_COMMAND_MSET_ERROR_METER[] = "mset_error_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HGET_TIMER[] = "hget_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HSET_TIMER[] = "hset_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HMGET_TIMER[] = "hmget_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HMSET_TIMER[] = "hmset_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HGETALL_TIMER[] = "hgetall_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HKEYS_TIMER[] = "hkeys_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HLEN_TIMER[] = "hlen_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HEXISTS_TIMER[] = "hexists_command";
constexpr char LASER_PROXY_METRIC_COMMAND_HDEL_TIMER[] = "hdel_command";
constexpr char LASER_PROXY_METRIC_COMMAND_DEL_TIMER[] = "del_command";
constexpr char LASER_PROXY_METRIC_COMMAND_EXPIRE_TIMER[] = "expire_command";
constexpr char LASER_PROXY_METRIC_COMMAND_EXPIREAT_TIMER[] = "expireat_command";
constexpr char LASER_PROXY_METRIC_COMMAND_TTL_TIMER[] = "ttl_command";
constexpr char LASER_PROXY_METRIC_COMMAND_DECR_TIMER[] = "decr_command";
constexpr char LASER_PROXY_METRIC_COMMAND_INCR_TIMER[] = "incr_command";
constexpr char LASER_PROXY_METRIC_COMMAND_DECRBY_TIMER[] = "decrby_command";
constexpr char LASER_PROXY_METRIC_COMMAND_INCRBY_TIMER[] = "incrby_command";
constexpr char LASER_PROXY_MGET_TOTAL_KEY_HIS[] = "mget_total_key_his";
constexpr char LASER_PROXY_MGET_NULL_VALUE_HIS[] = "mget_null_value_his";
constexpr char LASER_PROXY_HMGET_TOTAL_KEY_HIS[] = "hmget_total_key_his";
constexpr char LASER_PROXY_HMGET_NULL_VALUE_HIS[] = "hmget_null_value_his";
constexpr double LASER_PROXY_METRIC_CALL_BUCKET_SIZE = 1.0;
constexpr double LASER_PROXY_METRIC_CALL_MIN = 0.0;
constexpr double LASER_PROXY_METRIC_CALL_MAX = 1000.0;

RedisCommandProcess::RedisCommandProcess(std::shared_ptr<laser::LaserClient> laser_client,
                                         std::shared_ptr<laser::ProxyConfig> proxy_config) {
  // Init the config
  proxy_config_ = proxy_config;
  laser_client_ = laser_client;
  laser_proxy_redis_delimiter_first_ = FLAGS_redis_key_delimiter_first;
  laser_proxy_redis_delimiter_second_ = FLAGS_redis_key_delimiter_second;
  laser_client_option_.setMaxConnPerServer(FLAGS_laser_client_max_conn_pre_server);
  auto read_mode = laser::stringToClientRequestReadMode(FLAGS_laser_client_read_mode);
  if (read_mode) {
    laser_client_option_.setReadMode(*read_mode);
  }

  if (FLAGS_laser_proxy_local_first == true) {
    service_router::BalanceLocalFirstConfig local_first;
    local_first.setLocalIp(FLAGS_host);
    local_first.setDiffRange(FLAGS_laser_proxy_diff_range);
    laser_client_option_.setLocalFirstConfig(local_first);
    laser_client_option_.setLoadBalance(service_router::LoadBalanceMethod::LOCALFIRST);
  }

  // Init option table
  option_table_.insert({LASER_PROXY_DELIMITER_FIRST, Option::DELIMITER_FIRST});
  option_table_.insert({LASER_PROXY_DELIMITER_SECOND, Option::DELIMITER_SECOND});
  option_table_.insert({LASER_CLIENT_RECEIVE_TIMEOUT, Option::RECEIVE_TIMEOUT});
  option_table_.insert({LASER_CLIENT_READ_MODE, Option::READ_MODE});

  // Init metrics
  auto metrics = metrics::Metrics::getInstance();

  hget_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HGET_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  hset_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HSET_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  hmget_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HMGET_TIMER,
                                               LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                               LASER_PROXY_METRIC_CALL_MAX);
  hmset_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HMSET_TIMER,
                                               LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                               LASER_PROXY_METRIC_CALL_MAX);
  zadd_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_ZADD_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  zrangebyscore_command_timers_ = metrics->buildTimers(
      LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_ZRANGEBYSCORE_TIMER, LASER_PROXY_METRIC_CALL_BUCKET_SIZE,
      LASER_PROXY_METRIC_CALL_MIN, LASER_PROXY_METRIC_CALL_MAX);
  zremrangebyscore_command_timers_ = metrics->buildTimers(
      LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_ZREMRANGEBYSCORE_TIMER, LASER_PROXY_METRIC_CALL_BUCKET_SIZE,
      LASER_PROXY_METRIC_CALL_MIN, LASER_PROXY_METRIC_CALL_MAX);
  hgetall_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HGETALL_TIMER,
                                                 LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                 LASER_PROXY_METRIC_CALL_MAX);
  hkeys_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HKEYS_TIMER,
                                               LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                               LASER_PROXY_METRIC_CALL_MAX);
  hlen_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HLEN_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  hexists_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HEXISTS_TIMER,
                                                 LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                 LASER_PROXY_METRIC_CALL_MAX);
  hdel_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_HDEL_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  decr_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_DECR_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  incr_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_INCR_TIMER,
                                              LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                              LASER_PROXY_METRIC_CALL_MAX);
  decrby_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_DECRBY_TIMER,
                                                LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                LASER_PROXY_METRIC_CALL_MAX);
  incrby_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_INCRBY_TIMER,
                                                LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                LASER_PROXY_METRIC_CALL_MAX);
  mget_total_key_his_ = metrics->buildHistograms(LASER_PROXY_MODULE_NAME, LASER_PROXY_MGET_TOTAL_KEY_HIS,
                                                 LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                 LASER_PROXY_METRIC_CALL_MAX);
  mget_null_value_his_ = metrics->buildHistograms(LASER_PROXY_MODULE_NAME, LASER_PROXY_MGET_NULL_VALUE_HIS,
                                                  LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                  LASER_PROXY_METRIC_CALL_MAX);
  hmget_total_key_his_ = metrics->buildHistograms(LASER_PROXY_MODULE_NAME, LASER_PROXY_HMGET_TOTAL_KEY_HIS,
                                                  LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                  LASER_PROXY_METRIC_CALL_MAX);
  hmget_null_value_his_ = metrics->buildHistograms(LASER_PROXY_MODULE_NAME, LASER_PROXY_HMGET_NULL_VALUE_HIS,
                                                   LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                   LASER_PROXY_METRIC_CALL_MAX);
  del_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_DEL_TIMER,
                                             LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                             LASER_PROXY_METRIC_CALL_MAX);
  expire_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_EXPIRE_TIMER,
                                                LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                LASER_PROXY_METRIC_CALL_MAX);
  expireat_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_EXPIREAT_TIMER,
                                                  LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                  LASER_PROXY_METRIC_CALL_MAX);
  ttl_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_TTL_TIMER,
                                             LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                             LASER_PROXY_METRIC_CALL_MAX);
  exists_command_timers_ = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_EXISTS_TIMER,
                                                LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                LASER_PROXY_METRIC_CALL_MAX);
}

bool RedisCommandProcess::getLaserKey(laser::LaserKey* laser_key, const std::string& key) {
  if ((!FLAGS_laser_database_name.empty()) && (!FLAGS_laser_table_name.empty())) {
    laser_key->set_database_name(FLAGS_laser_database_name);
    laser_key->set_table_name(FLAGS_laser_table_name);
    std::vector<std::string> primary_key;
    primary_key.push_back(key);
    laser_key->set_primary_keys(primary_key);
  } else {
    // The req key format is:version0x01db_name0x01table_name0x01primary_key0x02primary_key0x01column_key0x02column_key
    std::vector<std::string> vec;
    folly::split(laser_proxy_redis_delimiter_first_, key, vec);
    if (vec.size() < REDIS_REQ_MIN_NUM) {
      VLOG(3) << "getLaserKey split the key fail.The key is:" << key;
      VLOG(3) << "laser_proxy_redis_delimiter_first_ : " << laser_proxy_redis_delimiter_first_;
      VLOG(3) << "laser_proxy_redis_delimiter_second_ : " << laser_proxy_redis_delimiter_second_;
      return false;
    }

    std::vector<std::string> primary_keys;
    std::vector<std::string> column_keys;
    folly::split(laser_proxy_redis_delimiter_second_, vec.at(REDIS_REQ_PRIMARY_KEY_LOCATION), primary_keys);

    if (vec.size() > REDIS_REQ_MIN_NUM) {
      folly::split(laser_proxy_redis_delimiter_second_, vec.at(REDIS_REQ_MIN_NUM), column_keys);
      laser_key->set_column_keys(column_keys);
    }

    laser_key->set_database_name(vec.at(REDIS_REQ_DATABASE_NAME_LOCATION));
    laser_key->set_table_name(vec.at(REDIS_REQ_TABLE_NAME_LOCATION));
    laser_key->set_primary_keys(primary_keys);
  }

  return true;
}

void RedisCommandProcess::setLaserClientOptionReceiveTimeout(laser::LaserKey laser_key, Operation operate) {
  if (operate == Operation::OPERATE_READ) {
    laser_client_option_.setReceiveTimeoutMs(proxy_config_->getReadTimeout(laser_key.get_database_name(),
                                                                           laser_key.get_table_name()));
  } else {
    laser_client_option_.setReceiveTimeoutMs(proxy_config_->getWriteTimeout(laser_key.get_database_name(),
                                                                            laser_key.get_table_name()));
  }
}

bool RedisCommandProcess::getLaserKV(laser::LaserKV* laser_kv, const std::string& key, const std::string& value) {
  laser::LaserKey laser_key;
  laser::LaserValue laser_value;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    VLOG(3) << "getLaserKV get laser key fail!";
    return false;
  }
  laser_value.set_string_value(value);
  laser_kv->set_key(laser_key);
  laser_kv->set_value(laser_value);

  return true;
}

bool RedisCommandProcess::configGet(std::string* res, const std::string& key) {
  auto option = option_table_.find(key);
  if (option == option_table_.end()) {
    return false;
  }
  switch (option->second) {
    case Option::DELIMITER_FIRST:
      *res = laser_proxy_redis_delimiter_first_;
      break;
    case Option::DELIMITER_SECOND:
      *res = laser_proxy_redis_delimiter_second_;
      break;
    case Option::RECEIVE_TIMEOUT:
      *res = std::to_string(laser_client_option_.getReceiveTimeoutMs());
      break;
    case Option::READ_MODE:
      *res = laser::toStringClientRequestReadMode(laser_client_option_.getReadMode());
      break;
    default:
      return false;
      break;
  }

  return true;
}

bool RedisCommandProcess::configSet(const std::string& key, const std::string& value) {
  VLOG(5) << "ConfigSetcmd the redis key:" << key << " value:" << value;
  auto option = option_table_.find(key);
  if (option == option_table_.end()) {
    return false;
  }
  switch (option->second) {
    case Option::DELIMITER_FIRST: {
      if (!value.empty()) {
        laser_proxy_redis_delimiter_first_ = value.at(0);
      } else {
        return false;
      }
      break;
    }
    case Option::DELIMITER_SECOND: {
      if (!value.empty()) {
        laser_proxy_redis_delimiter_second_ = value.at(0);
      } else {
        return false;
      }
      break;
    }
    case Option::RECEIVE_TIMEOUT: {
      auto timeout_ms = folly::tryTo<uint32_t>(value);
      if (timeout_ms.hasValue()) {
        laser_client_option_.setReceiveTimeoutMs(timeout_ms.value());
      } else {
        return false;
      }
      break;
    }
    case Option::READ_MODE: {
      auto read_mode = laser::stringToClientRequestReadMode(value);
      if (read_mode) {
        laser_client_option_.setReadMode(*read_mode);
      } else {
        return false;
      }
      break;
    }
    default:
      return false;
  }

  return true;
}

bool RedisCommandProcess::get(std::string* res, const std::string& key) {
  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_key.get_table_name()}};
  auto get_command_table_timers = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_GET_TIMER,
                                                       LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                       LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(get_command_table_timers.get());

  uint32_t allowed_value = proxy_config_->getAllowedFlow(laser_key.get_database_name(), laser_key.get_table_name());
  uint32_t random_value = folly::Random::rand32(1, 100);
  if (random_value > allowed_value) {
    *res = REDIS_RES_NULL;
    return true;
  }

  auto ret = laser_client_->getSync(laser_client_option_, res, laser_key);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client get is error,status is:" << statusToName(ret) << " key is:" << key;
  return false;
}

void RedisCommandProcess::setKeyReceiveTimeoutMs(const std::vector<std::string>& keys) {
  int32_t timeout = 0;

  for (uint32_t i = 0; i < keys.size(); i++) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, keys[i]);
    setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
    timeout = (timeout > laser_client_option_.getReceiveTimeoutMs()) ?
               timeout : laser_client_option_.getReceiveTimeoutMs();
  }
  laser_client_option_.setReceiveTimeoutMs(timeout);
}

void RedisCommandProcess::setKVReceiveTimeoutMs(const std::map<std::string, std::string>& kvs) {
  int32_t timeout = 0;

  for (auto& kv : kvs) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, kv.first);
    setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
    timeout = (timeout > laser_client_option_.getReceiveTimeoutMs()) ?
               timeout : laser_client_option_.getReceiveTimeoutMs();
  }
  laser_client_option_.setReceiveTimeoutMs(timeout);
}

bool RedisCommandProcess::mget(std::vector<std::string>* res, const std::vector<std::string>& keys) {
  mget_total_key_his_->addValue(keys.size());
  std::vector<laser::LaserKey> laser_keys(keys.size());
  for (uint32_t i = 0; i < keys.size(); i++) {
    auto result = getLaserKey(&(laser_keys[i]), keys[i]);
    if (result == false) {
      return false;
    }
  }
  setKeyReceiveTimeoutMs(keys);
  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_keys[0].get_table_name()}};
  auto mget_command_table_timers = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_MGET_TIMER,
                                                        LASER_PROXY_METRIC_CALL_BUCKET_SIZE,
                                                        LASER_PROXY_METRIC_CALL_MIN, LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(mget_command_table_timers.get());

  std::vector<LaserValue> laser_values;
  auto ret = laser_client_->mget(laser_client_option_, &laser_values, laser_keys);
  if (ret == laser::Status::OK) {
    uint32_t null_count = 0;
    for (auto& value : laser_values) {
      auto value_type = value.getType();
      if (laser::LaserValue::Type::string_value == value_type) {
        res->push_back(value.get_string_value());
      } else {
        null_count++;
        res->push_back(REDIS_RES_NULL);
        VLOG(3) << "The mget laser value type is null or map<string, string>,please check!";
      }
    }
    mget_null_value_his_->addValue(null_count);
    return true;
  }

  VLOG(3) << "The laser client mget is error,status is:" << statusToName(ret) << " first key:" << keys.at(0);
  return false;
}

bool RedisCommandProcess::set(const std::string& key, const std::string& value) {
  laser::LaserKV laser_kv;

  auto result = getLaserKV(&laser_kv, key, value);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_kv.get_key(), Operation::OPERATE_WRITE);
  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_kv.get_key().get_table_name()}};
  auto set_command_table_timers = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_SET_TIMER,
                                                       LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                       LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(set_command_table_timers.get());

  auto ret = laser_client_->setSync(laser_client_option_, laser_kv);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client set is error,status is:" << statusToName(ret) << " key is:" << key
          << " value is:" << value;
  return false;
}

bool RedisCommandProcess::setex(const std::string& key, int64_t milliseconds, const std::string& value) {
  laser::LaserKV laser_kv;

  auto result = getLaserKV(&laser_kv, key, value);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_kv.get_key(), Operation::OPERATE_WRITE);
  laser::LaserSetOption set_option;
  set_option.ttl = milliseconds;

  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_kv.get_key().get_table_name()}};
  auto set_command_table_timers = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_SETEX_TIMER,
                                                       LASER_PROXY_METRIC_CALL_BUCKET_SIZE, LASER_PROXY_METRIC_CALL_MIN,
                                                       LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(set_command_table_timers.get());

  auto ret = laser_client_->setxSync(laser_client_option_, laser_kv, set_option);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client set is error,status is:" << statusToName(ret) << " key is:" << key
          << " milliseconds is:" << milliseconds << " value is:" << value;
  return false;
}

bool RedisCommandProcess::append(uint32_t* length, const std::string& key, const std::string& value) {
  laser::LaserKey laser_key;

  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_key.get_table_name()}};
  auto append_command_table_timers = metrics->buildTimers(
      LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_APPEND_TIMER, LASER_PROXY_METRIC_CALL_BUCKET_SIZE,
      LASER_PROXY_METRIC_CALL_MIN, LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(append_command_table_timers.get());

  auto ret = laser_client_->appendSync(laser_client_option_, length, laser_key, value);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client set is error,status is:" << statusToName(ret) << " key is:" << key
          << " value is:" << value;
  return false;
}

bool RedisCommandProcess::exists(uint32_t* res, const std::vector<std::string>& keys) {
  metrics::Timer table_timer(exists_command_timers_.get());
  uint32_t count = 0;

  for (uint32_t i = 0; i < keys.size(); i++) {
    bool is_exist = false;
    laser::LaserKey laser_key;
    auto result = getLaserKey(&laser_key, keys[i]);
    if (result == false) {
      return false;
    }
    setKeyReceiveTimeoutMs(keys);
    auto ret = laser_client_->existSync(laser_client_option_, &is_exist, laser_key);
    if (ret == laser::Status::OK) {
      if (is_exist) {
        count++;
      }
    } else {
      VLOG(3) << "The laser client exist is error,status is:" << statusToName(ret) << " key:" << keys[i];
    }
  }

  *res = count;
  return true;
}

bool RedisCommandProcess::mset(std::vector<std::string>* res, const std::map<std::string, std::string>& kvs) {
  std::vector<laser::LaserKV> laser_kvs;

  for (auto& kv : kvs) {
    laser::LaserKV laser_kv;
    auto result = getLaserKV(&laser_kv, kv.first, kv.second);
    if (result == false) {
      return false;
    }
    laser_kvs.push_back(laser_kv);
  }
  setKVReceiveTimeoutMs(kvs);
  auto metrics = metrics::Metrics::getInstance();
  std::unordered_map<std::string, std::string> tags = {{"TableName", laser_kvs[0].get_key().get_table_name()}};
  auto mset_command_table_timers = metrics->buildTimers(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_MSET_TIMER,
                                                        LASER_PROXY_METRIC_CALL_BUCKET_SIZE,
                                                        LASER_PROXY_METRIC_CALL_MIN, LASER_PROXY_METRIC_CALL_MAX, tags);
  metrics::Timer table_timer(mset_command_table_timers.get());

  std::unordered_map<std::string, std::string> error_tags = {{"TableName", laser_kvs[0].get_key().get_table_name()}};
  auto mset_command_error_table_meter =
      metrics->buildMeter(LASER_PROXY_MODULE_NAME, LASER_PROXY_METRIC_COMMAND_MSET_ERROR_METER, tags);

  std::vector<int64_t> laser_results;
  auto ret = laser_client_->mset(laser_client_option_, &laser_results, laser_kvs);
  // Redis 默认mset 命令不会失败，总是返回ok，但是在laser环境下会出现出错或者部分kv设置出错的情况,
  // 这一版依据Laser是否全部设置成功返回true或者false.
  if (ret == laser::Status::OK) {
    for (auto& result : laser_results) {
      if (result == -1) {
        mset_command_error_table_meter->mark();
        return false;
      }
    }
    res->push_back(REDIS_RES_OK);
    return true;
  }

  VLOG(3) << "The laser client mset is error,status is:" << statusToName(ret) << " first key:" << kvs.begin()->first
          << " first value:" << kvs.begin()->second;

  mset_command_error_table_meter->mark();
  return false;
}

bool RedisCommandProcess::hget(std::string* res, const std::string& key, const std::string& field) {
  metrics::Timer timer(hget_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  auto ret = laser_client_->hgetSync(laser_client_option_, res, laser_key, field);
  if (ret == laser::Status::OK) {
    return true;
  }
  VLOG(3) << "The laser client hget is error,status is:" << statusToName(ret) << " key:" << key << " field:" << field;
  return false;
}

bool RedisCommandProcess::hset(const std::string& key, const std::string& field, const std::string& value) {
  metrics::Timer timer(hset_command_timers_.get());

  laser::LaserKey laser_key;

  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->hsetSync(laser_client_option_, laser_key, field, value);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client hset is error,status is:" << statusToName(ret) << " key:" << key << " field:" << field
          << " value:" << value;
  return false;
}

bool RedisCommandProcess::hmget(std::vector<std::string>* res, const std::string& key,
                                const std::vector<std::string>& fields) {
  metrics::Timer timer(hmget_command_timers_.get());
  hmget_total_key_his_->addValue(fields.size());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  std::map<std::string, std::string> values;
  auto ret = laser_client_->hmgetSync(laser_client_option_, &values, laser_key, fields);
  if (ret == laser::Status::OK) {
    uint32_t null_count = 0;
    for (auto& field : fields) {
      auto it = values.find(field);
      if (it != values.end()) {
        res->push_back(it->second);
      } else {
        null_count++;
        res->push_back(REDIS_RES_NULL);
        VLOG(3) << "The hmget laser value type is null,please check!"
                << " key is:" << key << "field is:" << field;
      }
    }
    mget_null_value_his_->addValue(null_count);
    return true;
  }

  VLOG(3) << "The laser client hmget is error,status is:" << statusToName(ret) << " key:" << key
          << " first field:" << fields.at(0);
  return false;
}

bool RedisCommandProcess::hmset(const std::string& key, const std::map<std::string, std::string>& fields) {
  metrics::Timer timer(hmset_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  laser::LaserValue laser_values;
  laser_values.set_map_value(fields);

  auto ret = laser_client_->hmsetSync(laser_client_option_, laser_key, laser_values);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client hmset is error,status is:" << statusToName(ret)
          << " first field:" << fields.begin()->first << " first value:" << fields.begin()->second;
  return false;
}

bool RedisCommandProcess::hgetall(std::map<std::string, std::string>* res, const std::string& key) {
  metrics::Timer timer(hgetall_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  std::map<std::string, std::string> values;
  auto ret = laser_client_->hgetallSync(laser_client_option_, &values, laser_key);
  if (ret == laser::Status::OK) {
    *res = values;
    return true;
  }

  VLOG(3) << "The laser client hgetall is error,status is:" << statusToName(ret) << " key:" << key;
  return false;
}

bool RedisCommandProcess::hkeys(std::vector<std::string>* res, const std::string& key) {
  metrics::Timer timer(hkeys_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  std::vector<std::string> fields;
  auto ret = laser_client_->hkeysSync(laser_client_option_, &fields, laser_key);
  if (ret == laser::Status::OK) {
    *res = fields;
    return true;
  }

  VLOG(3) << "The laser client hkeys is error,status is:" << statusToName(ret) << " key:" << key;
  return false;
}

bool RedisCommandProcess::hlen(uint32_t* res, const std::string& key) {
  metrics::Timer timer(hlen_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  uint32_t field_num;
  auto ret = laser_client_->hlenSync(laser_client_option_, &field_num, laser_key);
  if (ret == laser::Status::OK) {
    *res = field_num;
    return true;
  }

  VLOG(3) << "The laser client hlen is error,status is:" << statusToName(ret) << " key:" << key;
  return false;
}

bool RedisCommandProcess::hexists(const std::string& key, const std::string& field) {
  metrics::Timer timer(hexists_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  auto ret = laser_client_->hexistsSync(laser_client_option_, laser_key, field);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client hexists is error,status is:" << statusToName(ret) << " key:" << key
          << " field:" << field;
  return false;
}

bool RedisCommandProcess::hdel(const std::string& key, const std::string& field) {
  metrics::Timer timer(hdel_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->hdelSync(laser_client_option_, laser_key, field);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client hdel is error,status is:" << statusToName(ret) << " key:" << key << " field:" << field;
  return false;
}

bool RedisCommandProcess::del(uint32_t* res, const std::vector<std::string>& keys) {
  metrics::Timer timer(del_command_timers_.get());

  uint32_t count = 0;
  std::vector<laser::LaserKey> laser_keys(keys.size());
  for (uint32_t i = 0; i < keys.size(); i++) {
    laser::LaserKey laser_key;
    auto result = getLaserKey(&laser_key, keys[i]);
    if (result == false) {
      return false;
    }
    setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
    auto ret = laser_client_->delSync(laser_client_option_, laser_key);
    if (ret == laser::Status::OK) {
      count++;
    } else {
      VLOG(3) << "The laser client del is error,status is:" << statusToName(ret) << " key:" << keys[i];
    }
  }

  *res = count;
  return true;
}

bool RedisCommandProcess::expire(const std::string& key, int64_t time) {
  metrics::Timer timer(expire_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  int64_t time_ms = time * 1000;
  // 时间为负数则删除对应key
  if (time_ms < 0) {
    auto ret = laser_client_->delSync(laser_client_option_, laser_key);
    if (ret == laser::Status::OK) {
      return true;
    } else {
      VLOG(3) << "The laser client expire excute del command is error,status is:" << statusToName(ret) << " key:" << key
              << " time is:" << std::to_string(time);
      return false;
    }
  }
  auto ret = laser_client_->expireSync(laser_client_option_, laser_key, time_ms);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client expire is error,status is:" << statusToName(ret) << " key is:" << key
          << " time is:" << time;
  return false;
}

bool RedisCommandProcess::expireat(const std::string& key, int64_t time_at) {
  metrics::Timer timer(expireat_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  int64_t time_at_ms = time_at * 1000;
  // 小于当前时间则删除对应的key
  if (time_at_ms < static_cast<uint64_t>(common::currentTimeInMs())) {
    auto ret = laser_client_->delSync(laser_client_option_, laser_key);
    if (ret == laser::Status::OK) {
      return true;
    } else {
      VLOG(3) << "The laser client expireat excute del command is error,status is:" << statusToName(ret)
              << " key:" << key << " timestamp is:" << std::to_string(time_at);
      return false;
    }
  }

  auto ret = laser_client_->expireAtSync(laser_client_option_, laser_key, time_at_ms);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client expireat is error,status is:" << statusToName(ret) << " key is:" << key
          << " timestamp is:" << time_at;
  return false;
}

bool RedisCommandProcess::ttl(int64_t* res, const std::string& key) {
  metrics::Timer timer(ttl_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_READ);
  auto ret = laser_client_->ttlSync(laser_client_option_, res, laser_key);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client ttl is error,status is:" << statusToName(ret) << " key is:" << key;
  return false;
}

bool RedisCommandProcess::decr(int64_t* res, const std::string& key) {
  metrics::Timer timer(decr_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->decrSync(laser_client_option_, res, laser_key);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client decr is error,status is:" << statusToName(ret) << " key is:" << key;
  return false;
}

bool RedisCommandProcess::incr(int64_t* res, const std::string& key) {
  metrics::Timer timer(incr_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->incrSync(laser_client_option_, res, laser_key);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client incr is error,status is:" << statusToName(ret) << " key is:" << key;
  return false;
}

bool RedisCommandProcess::decrby(int64_t* res, const std::string& key, int64_t step) {
  metrics::Timer timer(decrby_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->decrBySync(laser_client_option_, res, laser_key, step);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client decrby is error,status is:" << statusToName(ret) << " key is:" << key
          << " step is:" << step;
  return false;
}

bool RedisCommandProcess::incrby(int64_t* res, const std::string& key, int64_t step) {
  metrics::Timer timer(incrby_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (result == false) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->incrBySync(laser_client_option_, res, laser_key, step);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client incrby is error,status is:" << statusToName(ret) << " key is:" << key
          << " step is:" << step;
  return false;
}

bool RedisCommandProcess::zadd(uint32_t* res, const std::string& key,
                               const std::unordered_map<std::string, double>& member_scores) {
  metrics::Timer timer(zadd_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->zaddSync(laser_client_option_, res, laser_key, member_scores);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client zadd is error,status is:" << statusToName(ret);
  return false;
}

bool RedisCommandProcess::zrangebyscore(std::vector<LaserFloatScoreMember>* res, const std::string& key, double min,
                                        double max) {
  metrics::Timer timer(zrangebyscore_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->zrangebyscoreSync(laser_client_option_, res, laser_key, min, max);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client zrangebyscore is error,status is:" << statusToName(ret);
  return false;
}

bool RedisCommandProcess::zremrangebyscore(uint32_t* res, const std::string& key, double min, double max) {
  metrics::Timer timer(zremrangebyscore_command_timers_.get());

  laser::LaserKey laser_key;
  auto result = getLaserKey(&laser_key, key);
  if (!result) {
    return false;
  }
  setLaserClientOptionReceiveTimeout(laser_key, Operation::OPERATE_WRITE);
  auto ret = laser_client_->zremrangebyscoreSync(laser_client_option_, res, laser_key, min, max);
  if (ret == laser::Status::OK) {
    return true;
  }

  VLOG(3) << "The laser client zremrangebyscore is error,status is:" << statusToName(ret);
  return false;
}

}  // namespace laser
