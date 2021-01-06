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

#include <float.h>
#include <limits.h>

#include "folly/Format.h"
#include "folly/String.h"
#include "wangle/bootstrap/ServerBootstrap.h"
#include "wangle/channel/AsyncSocketHandler.h"

#include "common/laser/config_manager.h"
#include "common/service_router/connection_pool.h"
#include "client/laser_client.h"

namespace laser {

using ProtocolErrorCallback = folly::Function<void()>;
using ProtocolWriteCallback = folly::Function<void(const std::string& msg)>;
using ProtocolLaserGetCallback = folly::Function<bool(std::string* res, const std::string& key)>;
using ProtocolLaserSetCallback = folly::Function<bool(const std::string& key, const std::string& value)>;
using ProtocolLaserSetexCallback =
    folly::Function<bool(const std::string& key, int64_t milliseconds, const std::string& value)>;
using ProtocolLaserAppendCallback =
    folly::Function<bool(uint32_t* length, const std::string& key, const std::string& value)>;
using ProtocolLaserExistsCallback = folly::Function<bool(uint32_t* res, const std::vector<std::string>& keys)>;
using ProtocolLaserMGetCallback =
    folly::Function<bool(std::vector<std::string>* res, const std::vector<std::string>& keys)>;
using ProtocolLaserMSetCallback =
    folly::Function<bool(std::vector<std::string>* res, const std::map<std::string, std::string>& kvs)>;
using ProtocolConfigGetCallback = folly::Function<bool(std::string* res, const std::string& key)>;
using ProtocolConfigSetCallback = folly::Function<bool(const std::string& key, const std::string& value)>;
using ProtocolLaserHGetCallback =
    folly::Function<bool(std::string* res, const std::string& key, const std::string& field)>;
using ProtocolLaserHSetCallback =
    folly::Function<bool(const std::string& key, const std::string& field, const std::string& value)>;
using ProtocolLaserHMGetCallback = folly::Function<
    bool(std::vector<std::string>* res, const std::string& key, const std::vector<std::string>& fields)>;  // NOLINT
using ProtocolLaserHMSetCallback =
    folly::Function<bool(const std::string& key, const std::map<std::string, std::string>& fvs)>;
using ProtocolLaserZAddCallback = folly::Function<
    bool(uint32_t* res, const std::string& key, const std::unordered_map<std::string, double> member_scores)>;  // NOLINT
using ProtocolLaserZRangeByScoreCallback =
    folly::Function<bool(std::vector<LaserFloatScoreMember>* res, const std::string& key, double min, double max)>;
using ProtocolLaserZRemRangeByScoreCallback =
    folly::Function<bool(uint32_t* res, const std::string& key, double min, double max)>;
using ProtocolLaserHGetAllCallback =
    folly::Function<bool(std::map<std::string, std::string>* res, const std::string& key)>;
using ProtocolLaserHKeysCallback = folly::Function<bool(std::vector<std::string>* res, const std::string& key)>;
using ProtocolLaserHLenCallback = folly::Function<bool(uint32_t* res, const std::string& key)>;
using ProtocolLaserHExistsCallback = folly::Function<bool(const std::string& key, const std::string& field)>;
using ProtocolLaserHDelCallback = folly::Function<bool(const std::string& key, const std::string& field)>;
using ProtocolLaserDelCallback = folly::Function<bool(uint32_t* res, const std::vector<std::string>& keys)>;
using ProtocolLaserExpireCallback = folly::Function<bool(const std::string& key, int64_t time)>;
using ProtocolLaserExpireAtCallback = folly::Function<bool(const std::string& key, int64_t time_at)>;
using ProtocolLaserTtlCallback = folly::Function<bool(int64_t* res, const std::string& key)>;
using ProtocolLaserDecrCallback = folly::Function<bool(int64_t* res, const std::string& key)>;
using ProtocolLaserIncrCallback = folly::Function<bool(int64_t* res, const std::string& key)>;
using ProtocolLaserDecrByCallback = folly::Function<bool(int64_t* res, const std::string& key, int64_t step)>;
using ProtocolLaserIncrByCallback = folly::Function<bool(int64_t* res, const std::string& key, int64_t step)>;

constexpr static uint32_t REDIS_REQ_VERSION_LOCATION = 0;
constexpr static uint32_t REDIS_REQ_DATABASE_NAME_LOCATION = 1;
constexpr static uint32_t REDIS_REQ_TABLE_NAME_LOCATION = 2;
constexpr static uint32_t REDIS_REQ_PRIMARY_KEY_LOCATION = 3;
constexpr static uint32_t REDIS_REQ_MIN_NUM = 4;
constexpr static uint32_t REDIS_COMMAND_ARGUMENTS_NUM_TWO = 2;
constexpr static uint32_t REDIS_COMMAND_ARGUMENTS_NUM_THREE = 3;
constexpr static uint32_t REDIS_COMMAND_ARGUMENTS_NUM_FOUR = 4;
constexpr static uint32_t REDIS_COMMAND_ARGUMENTS_NUM_FIVE = 5;
constexpr static char REDIS_RES_BULK_STRING[] = "$";
constexpr static char REDIS_RES_ARRAYS[] = "*";
constexpr static char REDIS_RES_INTEGER[] = ":";
constexpr static char REDIS_RES_EMPTY_LIST_OR_SET[] = "*0\r\n";
constexpr static char REDIS_RES_NEWLINE[] = "\r\n";
constexpr static char REDIS_RES_OK[] = "+OK\r\n";
constexpr static char REDIS_RES_NULL[] = "$-1\r\n";
constexpr static char REDIS_RES_PING[] = "+PONG\r\n";
constexpr static char REDIS_RES_INTEGER_ZERO[] = ":0\r\n";
constexpr static char REDIS_RES_INTEGER_ONE[] = ":1\r\n";
constexpr static char REDIS_RES_INTEGER_NEGATIVE_THREE[] = ":-3\r\n";
constexpr static char REDIS_RES_UNKONWN_CMD[] = "-Error unkonwn command\r\n";
constexpr static char REDIS_RES_UNKONWN_OPTION[] = "-Error unkonwn option\r\n";
constexpr static char REDIS_RES_FORMAT_ERROR[] = "-Error protocol format error\r\n";
constexpr static char REDIS_RES_MSET_FAILED[] = "-Error mset command failed, please set again\r\n";
constexpr static char REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER[] = "-Error wrong number of arguments\r\n";
constexpr static char REDIS_RES_HSET_FAILED[] = "-Error hset command failed, please set again\r\n";
constexpr static char REDIS_RES_HMSET_FAILED[] = "-Error hmset command failed, please set again\r\n";
constexpr static char REDIS_RES_SETEX_INVALID_EXPIRE_TIME[] = "-Error invalid expire time in setex\r\n";
constexpr static char REDIS_RES_PSETEX_INVALID_EXPIRE_TIME[] = "-Error invalid expire time in psetex\r\n";
constexpr static char REDIS_RES_VALUE_TYPE_ERROR[] = "-Error value is not an integer or out of range\r\n";
constexpr static char REDIS_RES_ZADD_FAILED[] = "-Error zadd command failed, please add again\r\n";
constexpr static char REDIS_RES_ZADD_VALUE_TYPE_ERROR[] = "-ERR value is not a valid double\r\n";
constexpr static char REDIS_RES_ZRANGEBYSCORE_COMMAND_ERROR[] = "-ERR syntax error\r\n";
constexpr static char REDIS_RES_ZRANGEBYSCORE_EMPTY_SET[] = "+(empty list or set)\r\n";
constexpr static char REDIS_RES_ZRANGEBYSCORE_MIN_OR_MAX_ERROR[] = "-ERR min or max is not double\r\n";
constexpr static char REDIS_COMMAND[] = "command";
constexpr static char REDIS_SELECT[] = "select";
constexpr static char REDIS_PING[] = "ping";
constexpr static char REDIS_TTL[] = "ttl";
constexpr static char REDIS_PTTL[] = "pttl";
constexpr static char REDIS_QUIT[] = "quit";
constexpr static char REDIS_ECHO[] = "echo";
constexpr static char REDIS_EXPIRE[] = "expire";
constexpr static char REDIS_EXPIREAT[] = "expireat";
constexpr static char REDIS_GET[] = "get";
constexpr static char REDIS_SET[] = "set";
constexpr static char REDIS_SETEX[] = "setex";
constexpr static char REDIS_PSETEX[] = "psetex";
constexpr static char REDIS_MGET[] = "mget";
constexpr static char REDIS_MSET[] = "mset";
constexpr static char REDIS_ZADD[] = "zadd";
constexpr static char REDIS_ZRANGEBYSCORE[] = "zrangebyscore";
constexpr static char REDIS_ZREMRANGEBYSCORE[] = "zremrangebyscore";
constexpr static char REDIS_CONFIG[] = "config";
constexpr static char REDIS_HGET[] = "hget";
constexpr static char REDIS_HSET[] = "hset";
constexpr static char REDIS_HMGET[] = "hmget";
constexpr static char REDIS_HMSET[] = "hmset";
constexpr static char REDIS_HGETALL[] = "hgetall";
constexpr static char REDIS_HKEYS[] = "hkeys";
constexpr static char REDIS_HLEN[] = "hlen";
constexpr static char REDIS_HEXISTS[] = "hexists";
constexpr static char REDIS_HDEL[] = "hdel";
constexpr static char REDIS_HVALS[] = "hvals";
constexpr static char REDIS_HSTRLEN[] = "hstrlen";
constexpr static char REDIS_HSETNX[] = "hsetnx";
constexpr static char REDIS_HINCRBY[] = "hincrby";
constexpr static char REDIS_HINCRBYFLOAT[] = "hincrbyfloat";
constexpr static char REDIS_APPEND[] = "append";
constexpr static char REDIS_GETSET[] = "getset";
constexpr static char REDIS_STRLEN[] = "strlen";
constexpr static char REDIS_SETNX[] = "setnx";
constexpr static char REDIS_MSETNX[] = "msetnx";
constexpr static char REDIS_DECR[] = "decr";
constexpr static char REDIS_INCR[] = "incr";
constexpr static char REDIS_DECRBY[] = "decrby";
constexpr static char REDIS_INCRBY[] = "incrby";
constexpr static char REDIS_INCRBYFLOAT[] = "incrbyfloat";

constexpr static char REDIS_DEL[] = "del";
constexpr static char REDIS_EXISTS[] = "exists";

class RedisProtocol {
 public:
  RedisProtocol();
  ~RedisProtocol();
  void process(wangle::HandlerAdapter<std::string>::Context* ctx, const std::string& msg);

  void setWriteCallback(ProtocolWriteCallback callback) { write_callback_ = std::move(callback); }
  void setErrorCallback(ProtocolErrorCallback callback) { error_callback_ = std::move(callback); }
  void setLaserGetCallback(ProtocolLaserGetCallback callback) { laser_get_callback_ = std::move(callback); }
  void setLaserSetCallback(ProtocolLaserSetCallback callback) { laser_set_callback_ = std::move(callback); }
  void setLaserSetexCallback(ProtocolLaserSetexCallback callback) { laser_setex_callback_ = std::move(callback); }
  void setLaserAppendCallback(ProtocolLaserAppendCallback callback) { laser_append_callback_ = std::move(callback); }
  void setLaserExistsCallback(ProtocolLaserExistsCallback callback) { laser_exists_callback_ = std::move(callback); }
  void setLaserMGetCallback(ProtocolLaserMGetCallback callback) { laser_mget_callback_ = std::move(callback); }
  void setLaserMSetCallback(ProtocolLaserMSetCallback callback) { laser_mset_callback_ = std::move(callback); }
  void setLaserConfigGetCallback(ProtocolConfigGetCallback callback) {
    laser_config_get_callback_ = std::move(callback);
  }
  void setLaserConfigSetCallback(ProtocolConfigSetCallback callback) {
    laser_config_set_callback_ = std::move(callback);
  }
  void setLaserHGetCallback(ProtocolLaserHGetCallback callback) { laser_hget_callback_ = std::move(callback); }
  void setLaserHSetCallback(ProtocolLaserHSetCallback callback) { laser_hset_callback_ = std::move(callback); }
  void setLaserHMGetCallback(ProtocolLaserHMGetCallback callback) { laser_hmget_callback_ = std::move(callback); }
  void setLaserHMSetCallback(ProtocolLaserHMSetCallback callback) { laser_hmset_callback_ = std::move(callback); }
  void setLaserZAddCallback(ProtocolLaserZAddCallback callback) { laser_zadd_callback_ = std::move(callback); }
  void setLaserZRangeByScoreCallback(ProtocolLaserZRangeByScoreCallback callback) {
    laser_zrangebyscore_callback_ = std::move(callback);
  }
  void setLaserZRemRangeByScoreCallback(ProtocolLaserZRemRangeByScoreCallback callback) {
    laser_zremrangebyscore_callback_ = std::move(callback);
  }
  void setLaserHGetAllCallback(ProtocolLaserHGetAllCallback callback) { laser_hgetall_callback_ = std::move(callback); }
  void setLaserHKeysCallback(ProtocolLaserHKeysCallback callback) { laser_hkeys_callback_ = std::move(callback); }
  void setLaserHLenCallback(ProtocolLaserHLenCallback callback) { laser_hlen_callback_ = std::move(callback); }
  void setLaserHExistsCallback(ProtocolLaserHExistsCallback callback) { laser_hexists_callback_ = std::move(callback); }
  void setLaserHDelCallback(ProtocolLaserHDelCallback callback) { laser_hdel_callback_ = std::move(callback); }
  void setLaserDelCallback(ProtocolLaserDelCallback callback) { laser_del_callback_ = std::move(callback); }
  void setLaserExpireCallback(ProtocolLaserExpireCallback callback) { laser_expire_callback_ = std::move(callback); }
  void setLaserExpireAtCallback(ProtocolLaserExpireAtCallback callback) {
    laser_expireat_callback_ = std::move(callback);
  }
  void setLaserTtlCallback(ProtocolLaserTtlCallback callback) { laser_ttl_callback_ = std::move(callback); }
  void setLaserDecrCallback(ProtocolLaserDecrCallback callback) { laser_decr_callback_ = std::move(callback); }
  void setLaserIncrCallback(ProtocolLaserIncrCallback callback) { laser_incr_callback_ = std::move(callback); }
  void setLaserDecrByCallback(ProtocolLaserDecrByCallback callback) { laser_decrby_callback_ = std::move(callback); }
  void setLaserIncrByCallback(ProtocolLaserIncrByCallback callback) { laser_incrby_callback_ = std::move(callback); }

  void configGetCmd(const std::string& key);
  void configSetCmd(const std::string& key, const std::string& value);
  void getCmd(const std::string& key);
  void setCmd(const std::string& key, const std::string& value);
  void setexCmd(const std::string& key, const int64_t seconds, const std::string& value);
  void psetexCmd(const std::string& key, const int64_t milliseconds, const std::string& value);
  void mgetCmd(const std::vector<std::string>& keys);
  void msetCmd(const std::map<std::string, std::string>& kvs);
  void hgetCmd(const std::string& key, const std::string& field);
  void hsetCmd(const std::string& key, const std::string& field, const std::string& value);
  void hmgetCmd(const std::string& key, const std::vector<std::string>& fields);
  void hmsetCmd(const std::string& key, const std::map<std::string, std::string>& fvs);
  void zaddCmd(const std::string& key, const std::unordered_map<std::string, double>& member_scores);
  void zrangebyscoreCmd(const std::string& key, const std::string& min, const std::string& max, bool flag_scores);
  void zremrangebyscoreCmd(const std::string& key, const std::string& min, const std::string& max);
  void hgetallCmd(const std::string& key);
  void hkeysCmd(const std::string& key);
  void hlenCmd(const std::string& key);
  void hexistsCmd(const std::string& key, const std::string& field);
  void hdelCmd(const std::string& key, const std::string& field);
  void hvalsCmd(const std::string& key);
  void hstrlenCmd(const std::string& key, const std::string& field);
  void hsetnxCmd(const std::string& key, const std::string& field, const std::string& value);
  void hincrbyCmd(const std::string& key, const std::string& field, int64_t step);
  void hincrbyfloatCmd(const std::string& key, const std::string& field, double step);
  void appendCmd(const std::string& key, const std::string& value);
  void getsetCmd(const std::string& key, const std::string& value);
  void strlenCmd(const std::string& key);
  void setnxCmd(const std::string& key, const std::string& value);
  void msetnxCmd(const std::map<std::string, std::string>& kvs);
  void delCmd(const std::vector<std::string>& keys);
  void expireCmd(const std::string& key, int64_t time);
  void expireAtCmd(const std::string& key, int64_t time_at);
  void ttlCmd(const std::string& key);
  void pttlCmd(const std::string& key);
  void existsCmd(const std::vector<std::string>& keys);
  void decrCmd(const std::string& key);
  void incrCmd(const std::string& key);
  void decrbyCmd(const std::string& key, int64_t step);
  void incrbyCmd(const std::string& key, int64_t step);
  void incrbyfloatCmd(const std::string& key, double step);

  void start(const std::string& msg);
  void store(const std::string& msg);
  void end(const std::string& msg);

  bool getMinAndMaxNum(double* num_min, double* num_max, bool* flag_min, bool* flag_max, const std::string& str_min,
                       const std::string& str_max);

  void appendString(std::string& res, const int32_t& len, const std::string& data);  // NOLINT
  void appendArray(std::string& res, const std::vector<std::string>& values);        // NOLINT
  bool appendScoreMember(std::string& res,                                           // NOLINT
                         const std::vector<LaserFloatScoreMember>& values, bool flag_scores, bool flag_min,
                         bool flag_max, double num_min, double num_max);
  void appendInteger(std::string& res, const int64_t& value);  // NOLINT
  enum class Command {
    COMMAND = 0,
    SELECT,
    PING,
    TTL,
    PTTL,
    QUIT,
    ECHO,
    EXPIRE,
    EXPIREAT,
    CONFIG,
    GET,
    SET,
    SETEX,
    PSETEX,
    APPEND,
    DECR,
    INCR,
    DECRBY,
    INCRBY,
    INCRBYFLOAT,
    GETSET,
    STRLEN,
    SETNX,
    MSETNX,
    MGET,
    MSET,
    ZADD,
    ZRANGEBYSCORE,
    ZREMRANGEBYSCORE,
    HGET,
    HSET,
    HMGET,
    HMSET,
    HGETALL,
    HKEYS,
    HLEN,
    HEXISTS,
    HDEL,
    HVALS,
    HSTRLEN,
    HSETNX,
    HINCRBY,
    HINCRBYFLOAT,
    DEL,
    EXISTS
  };

  enum class State {
    START_STATE = 0,
    CONTINUE_STATE,
  };

 private:
  int32_t cmd_num_{-1};
  uint32_t cmd_seq_{0};
  State cur_state_{State::START_STATE};
  std::vector<std::string> cmd_info_;
  std::map<const std::string, Command> cmd_table_;
  ProtocolErrorCallback error_callback_;
  ProtocolWriteCallback write_callback_;
  ProtocolConfigGetCallback laser_config_get_callback_;
  ProtocolConfigSetCallback laser_config_set_callback_;
  ProtocolLaserGetCallback laser_get_callback_;
  ProtocolLaserSetCallback laser_set_callback_;
  ProtocolLaserSetexCallback laser_setex_callback_;
  ProtocolLaserAppendCallback laser_append_callback_;
  ProtocolLaserExistsCallback laser_exists_callback_;
  ProtocolLaserMGetCallback laser_mget_callback_;
  ProtocolLaserMSetCallback laser_mset_callback_;
  ProtocolLaserZAddCallback laser_zadd_callback_;
  ProtocolLaserZRangeByScoreCallback laser_zrangebyscore_callback_;
  ProtocolLaserZRemRangeByScoreCallback laser_zremrangebyscore_callback_;
  ProtocolLaserHGetCallback laser_hget_callback_;
  ProtocolLaserHSetCallback laser_hset_callback_;
  ProtocolLaserHMGetCallback laser_hmget_callback_;
  ProtocolLaserHMSetCallback laser_hmset_callback_;
  ProtocolLaserHGetAllCallback laser_hgetall_callback_;
  ProtocolLaserHKeysCallback laser_hkeys_callback_;
  ProtocolLaserHLenCallback laser_hlen_callback_;
  ProtocolLaserHExistsCallback laser_hexists_callback_;
  ProtocolLaserHDelCallback laser_hdel_callback_;
  ProtocolLaserDelCallback laser_del_callback_;
  ProtocolLaserExpireCallback laser_expire_callback_;
  ProtocolLaserExpireAtCallback laser_expireat_callback_;
  ProtocolLaserTtlCallback laser_ttl_callback_;
  ProtocolLaserDecrCallback laser_decr_callback_;
  ProtocolLaserIncrCallback laser_incr_callback_;
  ProtocolLaserDecrByCallback laser_decrby_callback_;
  ProtocolLaserIncrByCallback laser_incrby_callback_;
};

}  // namespace laser
