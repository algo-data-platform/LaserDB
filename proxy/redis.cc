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

#include "common/service_router/router.h"
#include "common/service_router/thrift.h"
#include "common/laser/if/gen-cpp2/LaserService.h"
#include "common/laser/partition.h"

#include "redis.h"

namespace laser {
RedisProtocol::RedisProtocol() {
  // Init cmd table
  cmd_table_.insert({REDIS_COMMAND, Command::COMMAND});
  cmd_table_.insert({REDIS_SELECT, Command::SELECT});
  cmd_table_.insert({REDIS_PING, Command::PING});
  cmd_table_.insert({REDIS_TTL, Command::TTL});
  cmd_table_.insert({REDIS_PTTL, Command::PTTL});
  cmd_table_.insert({REDIS_QUIT, Command::QUIT});
  cmd_table_.insert({REDIS_ECHO, Command::ECHO});
  cmd_table_.insert({REDIS_EXPIRE, Command::EXPIRE});
  cmd_table_.insert({REDIS_EXPIREAT, Command::EXPIREAT});
  cmd_table_.insert({REDIS_CONFIG, Command::CONFIG});
  cmd_table_.insert({REDIS_GET, Command::GET});
  cmd_table_.insert({REDIS_SET, Command::SET});
  cmd_table_.insert({REDIS_APPEND, Command::APPEND});
  cmd_table_.insert({REDIS_DECR, Command::DECR});
  cmd_table_.insert({REDIS_INCR, Command::INCR});
  cmd_table_.insert({REDIS_DECRBY, Command::DECRBY});
  cmd_table_.insert({REDIS_INCRBY, Command::INCRBY});
  cmd_table_.insert({REDIS_INCRBYFLOAT, Command::INCRBYFLOAT});
  cmd_table_.insert({REDIS_GETSET, Command::GETSET});
  cmd_table_.insert({REDIS_STRLEN, Command::STRLEN});
  cmd_table_.insert({REDIS_SETEX, Command::SETEX});
  cmd_table_.insert({REDIS_SETNX, Command::SETNX});
  cmd_table_.insert({REDIS_MSETNX, Command::MSETNX});
  cmd_table_.insert({REDIS_PSETEX, Command::PSETEX});
  cmd_table_.insert({REDIS_MGET, Command::MGET});
  cmd_table_.insert({REDIS_MSET, Command::MSET});
  cmd_table_.insert({REDIS_ZADD, Command::ZADD});
  cmd_table_.insert({REDIS_ZRANGEBYSCORE, Command::ZRANGEBYSCORE});
  cmd_table_.insert({REDIS_ZREMRANGEBYSCORE, Command::ZREMRANGEBYSCORE});
  cmd_table_.insert({REDIS_HGET, Command::HGET});
  cmd_table_.insert({REDIS_HSET, Command::HSET});
  cmd_table_.insert({REDIS_HMGET, Command::HMGET});
  cmd_table_.insert({REDIS_HMSET, Command::HMSET});
  cmd_table_.insert({REDIS_HGETALL, Command::HGETALL});
  cmd_table_.insert({REDIS_HKEYS, Command::HKEYS});
  cmd_table_.insert({REDIS_HLEN, Command::HLEN});
  cmd_table_.insert({REDIS_HEXISTS, Command::HEXISTS});
  cmd_table_.insert({REDIS_HDEL, Command::HDEL});
  cmd_table_.insert({REDIS_HVALS, Command::HVALS});
  cmd_table_.insert({REDIS_HSTRLEN, Command::HSTRLEN});
  cmd_table_.insert({REDIS_HSETNX, Command::HSETNX});
  cmd_table_.insert({REDIS_HINCRBY, Command::HINCRBY});
  cmd_table_.insert({REDIS_HINCRBYFLOAT, Command::HINCRBYFLOAT});
  cmd_table_.insert({REDIS_DEL, Command::DEL});
  cmd_table_.insert({REDIS_EXISTS, Command::EXISTS});
}

RedisProtocol::~RedisProtocol() { cmd_table_.clear(); }

void RedisProtocol::process(wangle::HandlerAdapter<std::string>::Context* ctx, const std::string& msg) {
  // Dispatch the msg
  VLOG(5) << "Prccess msg is:" << msg;
  if (cur_state_ == State::START_STATE) {
    start(msg);
  } else if (cur_state_ == State::CONTINUE_STATE) {
    store(msg);
  }
}

void RedisProtocol::start(const std::string& msg) {
  if (msg[0] != '*') {
    write_callback_(REDIS_RES_FORMAT_ERROR);
    return;
  }
  auto t = folly::tryTo<std::make_signed<int32_t>::type>(msg.substr(1));
  cmd_num_ = t.value();
  cmd_info_.clear();
  VLOG(5) << "Process msg length is:" << cmd_num_;
  cur_state_ = State::CONTINUE_STATE;
}

void RedisProtocol::store(const std::string& msg) {
  cmd_seq_++;
  // 判断是否为偶数，协议中偶数位置为实际内容，奇数位上为内容长度
  if ((cmd_seq_ & 1) == 0) {
    cmd_info_.push_back(msg);
  } else {
    return;
  }

  if (cmd_info_.size() == cmd_num_) {
    end(msg);
    cmd_num_ = -1;
    cmd_seq_ = 0;
  }
}

void RedisProtocol::end(const std::string& msg) {
  folly::toLowerAscii(cmd_info_.front());
  VLOG(5) << "Process the cmd is:" << cmd_info_.front();
  std::vector<std::string> keys;
  std::map<std::string, std::string> kvs;
  auto cmd = cmd_table_.find(cmd_info_.front());
  if (cmd == cmd_table_.end()) {
    write_callback_(REDIS_RES_UNKONWN_CMD);
    cur_state_ = State::START_STATE;
    return;
  }

  switch (cmd->second) {
    case Command::COMMAND:
    case Command::SELECT:
      write_callback_(REDIS_RES_OK);
      break;
    case Command::PING:
      write_callback_(REDIS_RES_PING);
      break;
    case Command::TTL:
      // cmd_info 按序存放内容为: ttl key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        ttlCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::PTTL:
      // cmd_info 按序存放内容为: pttl key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        pttlCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::QUIT:
      write_callback_(REDIS_RES_OK);
      error_callback_();
      break;
    case Command::ECHO:
      // cmd_info 按序存放内容为:echo string
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        std::string res;
        appendString(res, cmd_info_.at(1).size(), cmd_info_.at(1));
        write_callback_(res);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::EXPIRE:
      // cmd_info 按序存放内容为: expire key seconds
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(2));
        if (t.hasValue()) {
          expireCmd(cmd_info_.at(1), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::EXPIREAT:
      // cmd_info 按序存放内容为: expireat key timestamp
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(2));
        if (t.hasValue()) {
          expireAtCmd(cmd_info_.at(1), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::CONFIG:
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        folly::toLowerAscii(cmd_info_.at(1));
        if (cmd_info_.at(1) == REDIS_SET) {
          // cmd_info 按序存放内容为:config set key value
          if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
            configSetCmd(cmd_info_.at(2), cmd_info_.at(3));
          } else {
            write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
          }
        } else if (cmd_info_.at(1) == REDIS_GET) {
          // cmd_info 按序存放内容为:config get key
          if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
            configGetCmd(cmd_info_.at(2));
          } else {
            write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
          }
        } else {
          write_callback_(REDIS_RES_UNKONWN_CMD);
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::GET:
      // cmd_info 按序存放内容为:get key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        getCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::SET:
      // cmd_info 按序存放内容为:set key value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        setCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::APPEND:
      // cmd_info 按序存放内容为:append key value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        appendCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::DECR:
      // cmd_info 按序存放内容为:decr key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        decrCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::INCR:
      // cmd_info 按序存放内容为:incr key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        incrCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::DECRBY:
      // cmd_info 按序存放内容为:decrby key decrement
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(2));
        if (t.hasValue()) {
          decrbyCmd(cmd_info_.at(1), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
          break;
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::INCRBY:
      // cmd_info 按序存放内容为:incrby key increment
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(2));
        if (t.hasValue()) {
          incrbyCmd(cmd_info_.at(1), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
          break;
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::INCRBYFLOAT:
      // cmd_info 按序存放内容为:incrbyfloat key increment
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        auto t = folly::tryTo<double>(cmd_info_.at(2));
        if (t.hasValue()) {
          incrbyfloatCmd(cmd_info_.at(1), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
          break;
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::GETSET:
      // cmd_info 按序存放内容为:getset key value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        getsetCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::STRLEN:
      // cmd_info 按序存放内容为:strlen key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        strlenCmd(cmd_info_.at(1));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::SETEX:
      // cmd_info 按序存放内容为:setex key seconds value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        auto t = folly::tryTo<std::make_signed<int32_t>::type>(cmd_info_.at(2));
        if ((t.hasValue()) && (t.value() >= 0)) {
          setexCmd(cmd_info_.at(1), t.value(), cmd_info_.at(3));
        } else {
          write_callback_(REDIS_RES_SETEX_INVALID_EXPIRE_TIME);
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::SETNX:
      // cmd_info 按序存放内容为:setnx key value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        setnxCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::MSETNX:
      // cmd_info 按序存放内容为:msetnx key value [key value...]
      if ((cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_THREE) && (cmd_info_.size() % 2 == 1)) {
        for (uint32_t i = 1; i < cmd_info_.size();) {
          kvs.insert({cmd_info_.at(i), cmd_info_.at(i + 1)});
          i += 2;
        }
        msetnxCmd(kvs);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::PSETEX:
      // cmd_info 按序存放内容为:psetex key milliseconds value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(2));
        if ((t.hasValue()) && (t.value() >= 0)) {
          psetexCmd(cmd_info_.at(1), t.value(), cmd_info_.at(3));
        } else {
          write_callback_(REDIS_RES_PSETEX_INVALID_EXPIRE_TIME);
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::MGET:
      // cmd_info 按序存放内容为:mget key [key...]
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        for (uint32_t i = 1; i < cmd_info_.size(); i++) {
          keys.push_back(cmd_info_.at(i));
        }
        mgetCmd(keys);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::MSET:
      // cmd_info 按序存放内容为:mset key value [key value...]
      if ((cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_THREE) && (cmd_info_.size() % 2 == 1)) {
        for (uint32_t i = 1; i < cmd_info_.size();) {
          kvs.insert({cmd_info_.at(i), cmd_info_.at(i + 1)});
          i += 2;
        }
        msetCmd(kvs);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::ZADD:
      // cmd_info 按序存放内容为:zadd key score member [score member ...].
      if ((cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_FOUR) && (cmd_info_.size() % 2 == 0)) {
        std::unordered_map<std::string, double> member_scores;
        for (uint32_t i = 2; i < cmd_info_.size();) {
          auto t = folly::tryTo<double>(cmd_info_.at(i));
          if (!t.hasValue()) {
            write_callback_(REDIS_RES_ZADD_VALUE_TYPE_ERROR);
            break;
          }
          member_scores.insert({cmd_info_.at(i + 1), t.value()});
          i += 2;
        }
        zaddCmd(cmd_info_.at(1), member_scores);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::ZRANGEBYSCORE:
      // cmd_info 按序存放内容为:zrangebyscore key [(]min [(]max [withscores] [LIMIT offset count].
      // cmd_info 按序存放内容为:zrangebyscore key [(]min [(]max [withscores] --- 暂不支持LIMIT参数.
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        bool flag_score = false;
        if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FIVE) {
          folly::toLowerAscii(cmd_info_.at(4));
          if (cmd_info_.at(4).compare("withscores")) {
            write_callback_(REDIS_RES_ZRANGEBYSCORE_COMMAND_ERROR);
          } else {
            flag_score = true;
          }
        }
        zrangebyscoreCmd(cmd_info_.at(1), cmd_info_.at(2), cmd_info_.at(3), flag_score);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::ZREMRANGEBYSCORE:
      // cmd_info 按序存放内容为:zremrangebyscore key [(]min [(]max.
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        std::string str_min = cmd_info_.at(2);
        std::string str_max = cmd_info_.at(3);
        zremrangebyscoreCmd(cmd_info_.at(1), str_min, str_max);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HGET:
      // cmd_info 按序存放内容为:hget key field
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        hgetCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HSET:
      // cmd_info 按序存放内容为:hset key field value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        hsetCmd(cmd_info_.at(1), cmd_info_.at(2), cmd_info_.at(3));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HMGET:
      // cmd_info 按序存放内容为:hmget key field1 field2 ...
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        std::vector<std::string> fields;
        for (uint32_t i = 2; i < cmd_info_.size(); i++) {
          fields.push_back(cmd_info_.at(i));
        }
        hmgetCmd(cmd_info_.at(1), fields);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HMSET:
      // cmd_info 按序存放内容为:hmset key field1 value1 field2 value2...
      if ((cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_FOUR) && (cmd_info_.size() % 2 == 0)) {
        std::map<std::string, std::string> fvs;
        for (uint32_t i = 2; i < cmd_info_.size();) {
          fvs.insert({cmd_info_.at(i), cmd_info_.at(i + 1)});
          i += 2;
        }
        hmsetCmd(cmd_info_.at(1), fvs);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HGETALL:
      // cmd_info 按序存放内容为:hgetall key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        hgetallCmd(msg);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HKEYS:
      // cmd_info 按序存放内容为:hkeys key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        hkeysCmd(msg);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HLEN:
      // cmd_info 按序存放内容为:hlen key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        hlenCmd(msg);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HEXISTS:
      // cmd_info 按序存放内容为:hexists key field
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        hexistsCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HDEL:
      // cmd_info 按序存放内容为:hdel key field
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        hdelCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HVALS:
      // cmd_info 按序存放内容为:hvals key
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        hvalsCmd(msg);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HSTRLEN:
      // cmd_info 按序存放内容为:hstrlen key field
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_THREE) {
        hstrlenCmd(cmd_info_.at(1), cmd_info_.at(2));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HSETNX:
      // cmd_info 按序存放内容为:hsetnx key field value
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        hsetnxCmd(cmd_info_.at(1), cmd_info_.at(2), cmd_info_.at(3));
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HINCRBY:
      // cmd_info 按序存放内容为:hincrby key field increment
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        auto t = folly::tryTo<std::make_signed<int64_t>::type>(cmd_info_.at(3));
        if (t.hasValue()) {
          hincrbyCmd(cmd_info_.at(1), cmd_info_.at(2), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
          break;
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::HINCRBYFLOAT:
      // cmd_info 按序存放内容为:hincrbyfloat key field increment
      if (cmd_info_.size() == REDIS_COMMAND_ARGUMENTS_NUM_FOUR) {
        auto t = folly::tryTo<double>(cmd_info_.at(3));
        if (t.hasValue()) {
          hincrbyfloatCmd(cmd_info_.at(1), cmd_info_.at(2), t.value());
        } else {
          write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
          break;
        }
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::DEL:
      // cmd_info 按序存放内容为:del key [key...]
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        for (uint32_t i = 1; i < cmd_info_.size(); i++) {
          keys.push_back(cmd_info_.at(i));
        }
        delCmd(keys);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    case Command::EXISTS:
      // cmd_info 按序存放内容为:exists key [key...]
      if (cmd_info_.size() >= REDIS_COMMAND_ARGUMENTS_NUM_TWO) {
        for (uint32_t i = 1; i < cmd_info_.size(); i++) {
          keys.push_back(cmd_info_.at(i));
        }
        existsCmd(keys);
      } else {
        write_callback_(REDIS_RES_ERROR_WRONG_ARGUMENTS_NUMBER);
      }
      break;
    default:
      write_callback_(REDIS_RES_UNKONWN_CMD);
      break;
  }
  cur_state_ = State::START_STATE;
}

void RedisProtocol::appendString(std::string& res, const int32_t& len, const std::string& data) {  // NOLINT
  res += REDIS_RES_BULK_STRING;
  res += std::to_string(len);
  res += REDIS_RES_NEWLINE;
  res += data;
  res += REDIS_RES_NEWLINE;
}

void RedisProtocol::appendArray(std::string& res, const std::vector<std::string>& values) {
  res += REDIS_RES_ARRAYS;
  res += std::to_string(values.size());
  res += REDIS_RES_NEWLINE;
  for (auto& value : values) {
    if (value != REDIS_RES_NULL) {
      appendString(res, value.size(), value);
    } else {
      res += REDIS_RES_NULL;
    }
  }
}

bool RedisProtocol::appendScoreMember(std::string& res, const std::vector<LaserFloatScoreMember>& values,
                                      bool flag_score, bool flag_min, bool flag_max, double num_min, double num_max) {
  bool ret = false;
  res += REDIS_RES_ARRAYS;
  uint32_t size = values.size();
  if (size <= 0) {
    return false;
  }
  if (!flag_min && (num_min == values.front().getScore())) {
    size -= 1;
  }

  if (!flag_max && (num_max == values.back().getScore())) {
    size -= 1;
  }

  if (size <= 0) {
    return false;
  }

  size = (flag_score) ? (size * 2) : size;
  res += std::to_string(size);
  res += REDIS_RES_NEWLINE;

  for (auto& value : values) {
    // flag_min flag_max false为有括号，不能取边界值，true为没有括号，可以取边界值
    if ((!flag_min && (num_min == value.getScore())) || (!flag_max && (num_max == value.getScore()))) {
      continue;
    }
    ret = true;
    double num_score = value.getScore();
    std::string str_score = std::to_string(num_score);
    appendString(res, value.getMember().size(), value.getMember());
    if (flag_score) {
      appendString(res, str_score.size(), str_score);
    }
  }

  return ret;
}
void RedisProtocol::appendInteger(std::string& res, const int64_t& value) {  // NOLINT
  res += REDIS_RES_INTEGER;
  res += std::to_string(value);
  res += REDIS_RES_NEWLINE;
}

void RedisProtocol::configGetCmd(const std::string& key) {
  VLOG(5) << "ConfigGetcmd the redis key is:" << key;
  std::string data;
  // Not register the function
  if (!laser_config_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_config_get_callback!!!";
    return;
  }

  auto ret = laser_config_get_callback_(&data, key);
  if (ret) {
    std::string res;
    std::vector<std::string> values;
    values.push_back(key);
    values.push_back(data);
    appendArray(res, values);
    write_callback_(res);
    VLOG(5) << "The config get key:" << key << " response data:" << data;
  } else {
    write_callback_(REDIS_RES_UNKONWN_OPTION);
    VLOG(3) << "Get NULL, the option is:" << key;
  }
}

void RedisProtocol::configSetCmd(const std::string& key, const std::string& value) {
  VLOG(5) << "ConfigSetcmd the redis key:" << key << " value:" << value;
  // Not register the function
  if (!laser_config_set_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_config_set_callback!!!";
    return;
  }

  auto ret = laser_config_set_callback_(key, value);
  if (ret) {
    write_callback_(REDIS_RES_OK);
  } else {
    write_callback_(REDIS_RES_UNKONWN_OPTION);
    VLOG(3) << "Set fail. The option is:" << key << " the value is:" << value;
  }
}

void RedisProtocol::getCmd(const std::string& key) {
  VLOG(5) << "Getcmd the redis key is:" << key;

  // Not register the function
  if (!laser_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_get_callback!!!";
    return;
  }

  std::string data;
  auto ret = laser_get_callback_(&data, key);
  if (ret) {
    std::string res;
    appendString(res, data.size(), data);
    VLOG(5) << "The redis response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Get NULL, the key is:" << key;
  }
}

void RedisProtocol::mgetCmd(const std::vector<std::string>& keys) {
  VLOG(5) << "MGetcmd the redis first key is:" << keys.at(0);

  // Not register the function
  if (!laser_mget_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_mget_callback!!!";
    return;
  }

  std::vector<std::string> values;

  auto ret = laser_mget_callback_(&values, keys);
  if (ret) {
    std::string res;
    appendArray(res, values);
    VLOG(5) << "The mget respone data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "MGet NULL, the first key is:" << keys.at(0);
  }
}

void RedisProtocol::setCmd(const std::string& key, const std::string& value) {
  VLOG(5) << "Setcmd the redis key/value:" << key << "/" << value;

  // NOT register the function
  if (!laser_set_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_set_callback!!!";
    return;
  }

  auto ret = laser_set_callback_(key, value);
  if (ret) {
    write_callback_(REDIS_RES_OK);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Set failed. The key is:" << key << " value is:" << value;
  }
}

void RedisProtocol::appendCmd(const std::string& key, const std::string& value) {
  VLOG(5) << "Appendcmd the redis key/value:" << key << "/" << value;

  // NOT register the function
  if (!laser_append_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_append_callback!!!";
    return;
  }

  uint32_t length = 0;
  auto ret = laser_append_callback_(&length, key, value);
  if (ret) {
    std::string res;
    appendInteger(res, length);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(3) << "Append  command failed. The key is:" << key << " value is:" << value;
  }
}

void RedisProtocol::decrCmd(const std::string& key) {
  VLOG(5) << "Decrcmd the redis key:" << key;

  // NOT register the function
  if (!laser_decr_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_decr_callback!!!";
    return;
  }

  int64_t result = 0;
  auto ret = laser_decr_callback_(&result, key);
  if (ret) {
    std::string res;
    appendInteger(res, result);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "Decr command failed. The key is:" << key;
  }
}

void RedisProtocol::incrCmd(const std::string& key) {
  VLOG(5) << "Incrcmd the redis key:" << key;

  // NOT register the function
  if (!laser_incr_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_incr_callback!!!";
    return;
  }

  int64_t result = 0;
  auto ret = laser_incr_callback_(&result, key);
  if (ret) {
    std::string res;
    appendInteger(res, result);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "Incr command failed. The key is:" << key;
  }
}

void RedisProtocol::decrbyCmd(const std::string& key, int64_t step) {
  VLOG(5) << "Decrbycmd the redis key:" << key << " step is:" << step;

  // NOT register the function
  if (!laser_decr_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_decr_callback!!!";
    return;
  }

  int64_t result = 0;
  auto ret = laser_decrby_callback_(&result, key, step);
  if (ret) {
    std::string res;
    appendInteger(res, result);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "Decrby command failed. The key is:" << key << " step is:" << step;
  }
}

void RedisProtocol::incrbyCmd(const std::string& key, int64_t step) {
  VLOG(5) << "Incrbycmd the redis key:" << key << " step is:" << step;

  // NOT register the function
  if (!laser_incr_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_incr_callback!!!";
    return;
  }

  int64_t result = 0;
  auto ret = laser_incrby_callback_(&result, key, step);
  if (ret) {
    std::string res;
    appendInteger(res, result);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "Incrby command failed. The key is:" << key << " step is:" << step;
  }
}

void RedisProtocol::incrbyfloatCmd(const std::string& key, double step) {
  VLOG(5) << "Incrbyfloatcmd the redis key:" << key << " step is:" << step;

  // NOT register the function
  if (!laser_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use incrbyfloat cmd, but not register laser_get_callback!!!";
    return;
  }

  // NOT register the function
  if (!laser_set_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use incrbyfloat cmd, but not register laser_set_callback!!!";
    return;
  }

  std::string data;
  double result = 0;
  auto get_ret = laser_get_callback_(&data, key);
  if (get_ret) {
    auto t = folly::tryTo<double>(data);
    if (t.hasValue()) {
      result = t.value() + step;
    } else {
      write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
      return;
    }
    VLOG(5) << "The incrbyfloat command get key:" << key << "response value is:" << data;
  } else {
    result = step;
  }

  std::string res;
  std::string result_string = std::to_string(result);
  appendString(res, result_string.size(), result_string);
  write_callback_(res);

  auto set_ret = laser_set_callback_(key, result_string);
  if (!set_ret) {
    VLOG(3) << "Incrbyfloat set failed. The key is:" << key << " value is:" << result;
  }
}

void RedisProtocol::getsetCmd(const std::string& key, const std::string& value) {
  VLOG(5) << "Getsetcmd the redis key/value:" << key << "/" << value;

  // NOT register the function
  if (!laser_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use getset cmd, but not register laser_get_callback!!!";
    return;
  }

  // NOT register the function
  if (!laser_set_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use getset cmd, but not register laser_set_callback!!!";
    return;
  }

  std::string data;
  auto get_ret = laser_get_callback_(&data, key);
  if (get_ret) {
    std::string res;
    // TODO(liujunpeng) :Redis协议中需要判断返回的data类型是否为string，非string类型返回错误
    // 但laser中value类型已全部为string
    appendString(res, data.size(), data);
    VLOG(5) << "The Getset command get redis response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Getset get NULL, the key is:" << key;
  }

  auto set_ret = laser_set_callback_(key, value);
  if (!set_ret) {
    VLOG(3) << "Getset set failed. The key is:" << key << " value is:" << value;
  }
}

void RedisProtocol::strlenCmd(const std::string& key) {
  VLOG(5) << "Strlen the redis key:" << key;

  // NOT register the function
  if (!laser_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use strlen cmd, but not register laser_get_callback!!!";
    return;
  }

  std::string data;
  auto get_ret = laser_get_callback_(&data, key);

  if (get_ret) {
    std::string res;
    // TODO(liujunpeng) :Redis协议中需要判断返回的data类型是否为string，非string类型返回错误
    // 但laser中value类型已全部为string
    appendInteger(res, data.size());
    VLOG(5) << "The Strlen command get redis response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(3) << "Strlen get NULL, the key is:" << key;
  }
}
void RedisProtocol::setexCmd(const std::string& key, int64_t seconds, const std::string& value) {
  VLOG(5) << "Setexcmd the redis key/seconds/value:" << key << "/" << seconds << "/" << value;

  // NOT register the function
  if (!laser_setex_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_setex_callback!!!";
    return;
  }
  auto milliseconds = seconds * 1000;
  auto ret = laser_setex_callback_(key, milliseconds, value);
  if (ret) {
    write_callback_(REDIS_RES_OK);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Setex command failed. The key is:" << key << " seconds:" << seconds << " value is:" << value;
  }
}

void RedisProtocol::setnxCmd(const std::string& key, const std::string& value) {
  VLOG(5) << "Setnxcmd the redis key/value:" << key << "/" << value;

  // NOT register the function
  if (!laser_get_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use setnx cmd, but not register laser_get_callback!!!";
    return;
  }

  // NOT register the function
  if (!laser_set_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use setnx cmd, but not register laser_set_callback!!!";
    return;
  }

  std::string data;
  auto get_ret = laser_get_callback_(&data, key);
  if (get_ret) {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(5) << "Setnx failed.The key is exist.The key is:" << key << " value is:" << value
            << "the get value is:" << data;
    return;
  }

  auto set_ret = laser_set_callback_(key, value);
  if (set_ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Setnx execute set command failed. The key is:" << key << " value is:" << value;
  }
}

void RedisProtocol::msetnxCmd(const std::map<std::string, std::string>& kvs) {
  VLOG(5) << "MSetnxcmd the redis first key is:" << kvs.begin()->first << " value is:" << kvs.begin()->second;

  // Not register the function
  if (!laser_mget_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use msetnx cmd,but not register laser_mget_callback!!!";
    return;
  }

  // Not register the function
  if (!laser_mset_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use msetnx cmd, but not register laser_mset_callback!!!";
    return;
  }

  std::vector<std::string> keys;
  for (auto& kv : kvs) {
    keys.push_back(kv.first);
  }

  std::vector<std::string> values;
  auto mget_ret = laser_mget_callback_(&values, keys);
  if (mget_ret) {
    uint32_t null_value_count = 0;
    for (auto& value : values) {
      if (value == REDIS_RES_NULL) {
        null_value_count++;
      }
    }

    if (null_value_count < keys.size()) {
      write_callback_(REDIS_RES_INTEGER_ZERO);
      return;
    }
  }

  std::vector<std::string> data;
  auto mset_ret = laser_mset_callback_(&data, kvs);
  if (mset_ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_MSET_FAILED);
    VLOG(3) << "MSetnx execute mset command failed, the first key is:" << kvs.at(0);
  }
}

void RedisProtocol::psetexCmd(const std::string& key, int64_t milliseconds, const std::string& value) {
  VLOG(5) << "Psetexcmd the redis key/milliseconds/value:" << key << "/" << milliseconds << "/" << value;

  // NOT register the function
  if (!laser_setex_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use psetex smd, but Not register laser_setex_callback!!!";
    return;
  }

  auto ret = laser_setex_callback_(key, milliseconds, value);
  if (ret) {
    write_callback_(REDIS_RES_OK);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Psetex execute setex command failed. The key is:" << key << " milliseconds:" << milliseconds
            << " value is:" << value;
  }
}

void RedisProtocol::msetCmd(const std::map<std::string, std::string>& kvs) {
  VLOG(5) << "MSetcmd the redis first key is:" << kvs.begin()->first << " value is:" << kvs.begin()->second;

  // Not register the function
  if (!laser_mset_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_mset_callback!!!";
    return;
  }

  std::vector<std::string> data;

  auto ret = laser_mset_callback_(&data, kvs);
  if (ret) {
    std::string res;
    res += REDIS_RES_OK;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_MSET_FAILED);
    VLOG(3) << "MSet failed, the first key is:" << kvs.begin()->first << " value is:" << kvs.begin()->second;
  }
}

void RedisProtocol::zaddCmd(const std::string& key, const std::unordered_map<std::string, double>& member_scores) {
  VLOG(5) << "Zaddcmd the redis key is:" << key;

  // Not register the function
  if (!laser_zadd_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_zadd_callback.";
    return;
  }

  uint32_t res_number = 0;
  auto ret = laser_zadd_callback_(&res_number, key, member_scores);

  if (ret) {
    std::string res;
    appendInteger(res, res_number);
    VLOG(5) << "The zadd response res:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_ZADD_FAILED);
    VLOG(3) << "zadd failed, the key is : " << key << " , first member is : " << member_scores.begin()->first
            << " , first score is:" << member_scores.begin()->second;
  }
}

bool RedisProtocol::getMinAndMaxNum(double* num_min, double* num_max, bool* flag_min, bool* flag_max,
                                    const std::string& str_min, const std::string& str_max) {
  folly::Expected<double, folly::ConversionCode> t;
  std::string fix_min("-inf");
  std::string fix_max("+inf");

  if (fix_min.compare(str_min)) {
    if ('(' == str_min[0]) {
      *flag_min = false;
      t = folly::tryTo<double>(str_min.substr(1));
    } else {
      *flag_min = true;
      t = folly::tryTo<double>(str_min);
    }
    if (t.hasValue()) {
      *num_min = t.value();
    } else {
      VLOG(3) << "get num_min failed.";
      return false;
    }
  } else {
    *flag_min = true;
    *num_min = -(FLT_MAX / LASER_FLOAT_AMPLIFICATION_FACTOR);
  }

  if (fix_max.compare(str_max)) {
    if ('(' == str_max[0]) {
      *flag_max = false;
      t = folly::tryTo<double>(str_max.substr(1));
    } else {
      *flag_max = true;
      t = folly::tryTo<double>(str_max);
    }
    if (t.hasValue()) {
      *num_max = t.value();
    } else {
      VLOG(3) << "get num_max failed.";
      return false;
    }
  } else {
    *flag_max = true;
    *num_max = FLT_MAX / LASER_FLOAT_AMPLIFICATION_FACTOR;
  }

  return true;
}

void RedisProtocol::zrangebyscoreCmd(const std::string& key, const std::string& min, const std::string& max,
                                     bool flag_score) {
  VLOG(5) << "zrangebyscore the redis key is:" << key;

  // Not register the function
  if (!laser_zrangebyscore_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_zrangebyscore_callback!!!";
    return;
  }

  double num_min;
  double num_max;
  bool status = true;
  bool flag_min = true;
  bool flag_max = true;
  std::vector<LaserFloatScoreMember> values;
  status = getMinAndMaxNum(&num_min, &num_max, &flag_min, &flag_max, min, max);
  VLOG(5) << "The zrangebyscore min is:" << num_min << " max is:" << num_max << " flag min:" << flag_min
          << " flag max:" << flag_max;
  if (!status) {
    write_callback_(REDIS_RES_ZRANGEBYSCORE_MIN_OR_MAX_ERROR);
    return;
  }
  if (num_min > num_max) {
    write_callback_(REDIS_RES_ZRANGEBYSCORE_EMPTY_SET);
    return;
  }

  auto ret = laser_zrangebyscore_callback_(&values, key, num_min, num_max);
  if (ret) {
    std::string res;
    bool has_value = appendScoreMember(res, values, flag_score, flag_min, flag_max, num_min, num_max);
    if (!has_value) {
      write_callback_(REDIS_RES_ZRANGEBYSCORE_EMPTY_SET);
      VLOG(3) << "The zrangebyscore result is empty set the key is:" << key << " min is:" << num_min
              << " max is:" << num_max << " the value size is:" << values.size();
      return;
    }

    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_ZRANGEBYSCORE_EMPTY_SET);
    VLOG(3) << "zrangebyscore failed, the key is:" << key << " min is:" << min << " max is:" << max;
  }
}

void RedisProtocol::zremrangebyscoreCmd(const std::string& key, const std::string& str_min,
                                        const std::string& str_max) {
  VLOG(5) << "zremrangebyscore the redis key is:" << key;

  // Not register the function
  if (!laser_zremrangebyscore_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_zremrangebyscore_callback!!!";
    return;
  }

  double num_min;
  double num_max;
  uint32_t res_number = 0;
  bool status = true;
  bool flag_min = true;
  bool flag_max = true;
  status = getMinAndMaxNum(&num_min, &num_max, &flag_min, &flag_max, str_min, str_max);
  VLOG(5) << "The zremrangebyscore min is:" << num_min << " max is:" << num_max << " flag min:" << flag_min
          << " flag max:" << flag_max;
  if (!status) {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    return;
  }
  if (num_min > num_max) {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    return;
  }

  auto ret = laser_zremrangebyscore_callback_(&res_number, key, num_min, num_max);
  if (ret) {
    std::string res;
    appendInteger(res, res_number);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(3) << "zremrangebyscore failed, the key is:" << key << " min is:" << str_min << " max is:" << str_max;
  }
}

void RedisProtocol::hgetCmd(const std::string& key, const std::string& field) {
  VLOG(5) << "HGetcmd the redis key is:" << key << " field is:" << field;

  // Not register the function
  if (!laser_hget_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hget_callback!!!";
    return;
  }

  std::string data;
  auto ret = laser_hget_callback_(&data, key, field);
  if (ret) {
    std::string res;
    appendString(res, data.size(), data);
    VLOG(5) << "The hget response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "HGet NULL, the key is:" << key << " field is:" << field;
  }
}

void RedisProtocol::hsetCmd(const std::string& key, const std::string& field, const std::string& value) {
  VLOG(5) << "HGetcmd the redis key is:" << key << " field is:" << field << " value is:" << value;

  // Not register the function
  if (!laser_hset_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hset_callback!!!";
    return;
  }

  std::string data;
  auto ret = laser_hset_callback_(key, field, value);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    // Redis中hset命令不会失败,只有创建返回“1”，更新返回“0”,这里Laser写入失败是返回REDIS_RES_HSET_FAILED
    write_callback_(REDIS_RES_HSET_FAILED);
    VLOG(3) << "HSet failed, the key is:" << key << " field is:" << field;
  }
}

void RedisProtocol::hmgetCmd(const std::string& key, const std::vector<std::string>& fields) {
  VLOG(5) << "HMGetcmd the redis key is:" << key << " first field is:" << fields.at(0);

  // Not register the function
  if (!laser_hmget_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hmget_callback!!!";
    return;
  }

  std::vector<std::string> values;
  auto ret = laser_hmget_callback_(&values, key, fields);
  if (ret) {
    std::string res;
    appendArray(res, values);
    VLOG(5) << "The hmget response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "HMGet NULL, the key is:" << key << " the first key is:" << fields.at(0);
  }
}

void RedisProtocol::hmsetCmd(const std::string& key, const std::map<std::string, std::string>& fvs) {
  VLOG(5) << "HMSetcmd the redis key is:" << key << " first field is:" << fvs.begin()->first
          << " first value is:" << fvs.begin()->second;

  // Not register the function
  if (!laser_hmset_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hmset_callback!!!";
    return;
  }

  auto ret = laser_hmset_callback_(key, fvs);
  if (ret) {
    write_callback_(REDIS_RES_OK);
  } else {
    write_callback_(REDIS_RES_HMSET_FAILED);
    VLOG(3) << "HMSet failed, the key is:" << key << " the first field is:" << fvs.begin()->first
            << " first value is:" << fvs.begin()->second;
  }
}

void RedisProtocol::hgetallCmd(const std::string& key) {
  VLOG(5) << "HGetAllcmd the key is:" << key;

  // Not register the function
  if (!laser_hgetall_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hgetall_callback!!!";
    return;
  }

  std::map<std::string, std::string> fvs;
  auto ret = laser_hgetall_callback_(&fvs, key);
  if (ret) {
    std::string res;
    std::vector<std::string> values;
    for (auto it = fvs.begin(); it != fvs.end(); it++) {
      values.push_back(it->first);
      values.push_back(it->second);
    }
    appendArray(res, values);
    write_callback_(res);
    VLOG(5) << "The hgetall response data:" << res;
  } else {
    write_callback_(REDIS_RES_EMPTY_LIST_OR_SET);
    VLOG(3) << "HGetAll NULL, the key is:" << key;
  }
}

void RedisProtocol::hkeysCmd(const std::string& key) {
  VLOG(5) << "HKeyscmd the key is:" << key;

  // Not register the function
  if (!laser_hkeys_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hkeys_callback!!!";
    return;
  }

  std::vector<std::string> fields;
  auto ret = laser_hkeys_callback_(&fields, key);
  if (ret) {
    std::string res;
    appendArray(res, fields);
    write_callback_(res);
    VLOG(5) << "The hkeys response data:" << res;
  } else {
    write_callback_(REDIS_RES_EMPTY_LIST_OR_SET);
    VLOG(3) << "HKeys is NULL, the key is:" << key;
  }
}

void RedisProtocol::hlenCmd(const std::string& key) {
  VLOG(5) << "HLencmd the key is:" << key;

  // Not register the function
  if (!laser_hlen_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hlen_callback!!!";
    return;
  }

  uint32_t fields_num;
  auto ret = laser_hlen_callback_(&fields_num, key);
  if (ret) {
    std::string res;
    appendInteger(res, fields_num);
    write_callback_(res);
    VLOG(5) << "The heln response data:" << res;
  } else {
    write_callback_(REDIS_RES_FORMAT_ERROR);
    VLOG(3) << "HLen is error, the key is:" << key;
  }
}

void RedisProtocol::hexistsCmd(const std::string& key, const std::string& field) {
  VLOG(5) << "HExistscmd the key is:" << key << " field is:" << field;

  // Not register the function
  if (!laser_hexists_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hexists_callback!!!";
    return;
  }

  auto ret = laser_hexists_callback_(key, field);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  }
}

void RedisProtocol::hdelCmd(const std::string& key, const std::string& field) {
  VLOG(5) << "HDelcmd the key is:" << key << " field is:" << field;

  // Not register the function
  if (!laser_hdel_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_hdel_callback!!!";
    return;
  }

  auto ret = laser_hdel_callback_(key, field);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  }
}

void RedisProtocol::hvalsCmd(const std::string& key) {
  VLOG(5) << "Hvalscmd the key is:" << key;

  // Not register the function
  if (!laser_hgetall_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use hvalsCmd, but not register laser_hgetall_callback!!!";
    return;
  }

  std::map<std::string, std::string> fvs;
  auto ret = laser_hgetall_callback_(&fvs, key);
  if (ret) {
    std::string res;
    std::vector<std::string> values;
    for (auto it = fvs.begin(); it != fvs.end(); it++) {
      values.push_back(it->second);
    }
    appendArray(res, values);
    write_callback_(res);
    VLOG(5) << "The hvals response data:" << res;
  } else {
    write_callback_(REDIS_RES_EMPTY_LIST_OR_SET);
    VLOG(3) << "HVals NULL, the key is:" << key;
  }
}

void RedisProtocol::hstrlenCmd(const std::string& key, const std::string& field) {
  VLOG(5) << "HStrlencmd the redis key is:" << key << " field is:" << field;

  // Not register the function
  if (!laser_hget_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use hstrlenCmd, but not register laser_hget_callback!!!";
    return;
  }

  std::string data;
  auto ret = laser_hget_callback_(&data, key, field);
  if (ret) {
    std::string res;
    appendInteger(res, data.size());
    VLOG(5) << "The hstrlen response data:" << res;
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(3) << "HStrlen NULL, the key is:" << key << " field is:" << field;
  }
}

void RedisProtocol::hsetnxCmd(const std::string& key, const std::string& field, const std::string& value) {
  VLOG(5) << "HSetnxcmd the redis key is:" << key << " field is:" << field << " value is:" << value;

  // Not register the function
  if ((!laser_hset_callback_) && (!laser_hexists_callback_)) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use hsetnxCmd, but not register laser_hset_callback or laser_hexists_callback!!!";
    return;
  }

  auto ret = laser_hexists_callback_(key, field);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  } else {
    auto hset_ret = laser_hset_callback_(key, field, value);
    if (hset_ret) {
      write_callback_(REDIS_RES_INTEGER_ONE);
    } else {
      write_callback_(REDIS_RES_INTEGER_ZERO);
      VLOG(3) << "HSetnx failed, the key is:" << key << " field is:" << field << " value is:" << value;
    }
  }
}

void RedisProtocol::hincrbyCmd(const std::string& key, const std::string& field, int64_t step) {
  VLOG(5) << "Incrbycmd the redis key:" << key << " field is:" << field << " step is:" << step;

  // NOT register the function
  if ((!laser_hset_callback_) && (!laser_hget_callback_)) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use hincrbyCmd, but not register laser_set_callback or laser_get_callback_!!!";
    return;
  }

  int64_t result;
  std::string value;
  auto ret = laser_hget_callback_(&value, key, field);
  if (ret) {
    auto t = folly::tryTo<std::make_signed<int64_t>::type>(value);
    if (!t.hasValue()) {
      write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
      return;
    }
    result = t.value() + step;
  } else {
    result = step;
  }

  auto hset_ret = laser_hset_callback_(key, field, std::to_string(result));
  if (hset_ret) {
    std::string res;
    appendInteger(res, result);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "HIncrby command execute hset failed. The key is:" << key << " field is:" << field
            << " step is:" << step;
  }
}

void RedisProtocol::hincrbyfloatCmd(const std::string& key, const std::string& field, double step) {
  VLOG(5) << "Incrbyfloatcmd the redis key:" << key << " field is:" << field << " step is:" << step;

  // NOT register the function
  if ((!laser_hset_callback_) && (!laser_hget_callback_)) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Use hincrbyfloatCmd, but not register laser_set_callback or laser_get_callback_!!!";
    return;
  }

  double result;
  std::string value;
  auto ret = laser_hget_callback_(&value, key, field);
  if (ret) {
    auto t = folly::tryTo<double>(value);
    if (!t.hasValue()) {
      write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
      return;
    }
    result = t.value() + step;
  } else {
    result = step;
  }

  auto hset_ret = laser_hset_callback_(key, field, std::to_string(result));
  if (hset_ret) {
    std::string res;
    std::string result_string = std::to_string(result);
    appendString(res, result_string.size(), result_string);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_VALUE_TYPE_ERROR);
    VLOG(3) << "HIncrbyfloat command execute hset failed. The key is:" << key << " field is:" << field
            << " step is:" << step;
  }
}

void RedisProtocol::delCmd(const std::vector<std::string>& keys) {
  VLOG(5) << "Delcmd the first key is:" << keys.at(0);

  // Not register the function
  if (!laser_del_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_del_callback!!!";
    return;
  }

  uint32_t count = 0;
  auto ret = laser_del_callback_(&count, keys);
  if (ret) {
    std::string res;
    appendInteger(res, count);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  }
}

void RedisProtocol::expireCmd(const std::string& key, int64_t time) {
  VLOG(5) << "Expirecmd the key is:" << key << " time is:" << std::to_string(time);

  // Not register the function
  if (!laser_expire_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_expire_callback!!!";
    return;
  }

  auto ret = laser_expire_callback_(key, time);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  }
}

void RedisProtocol::expireAtCmd(const std::string& key, int64_t time_at) {
  VLOG(5) << "Expireatcmd the key is:" << key << " time is:" << std::to_string(time_at);

  // Not register the function
  if (!laser_expireat_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_expireat_callback!!!";
    return;
  }

  auto ret = laser_expireat_callback_(key, time_at);
  if (ret) {
    write_callback_(REDIS_RES_INTEGER_ONE);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
  }
}

void RedisProtocol::ttlCmd(const std::string& key) {
  VLOG(5) << "Ttlcmd the key is:" << key;

  // Not register the function
  if (!laser_ttl_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_ttl_callback!!!";
    return;
  }

  int64_t ttl;
  auto ret = laser_ttl_callback_(&ttl, key);
  if (ret) {
    std::string res;
    int64_t ttl_seconds = 0;
    if (ttl >= 1000) {
      ttl_seconds = ttl / 1000;
    } else if (ttl >= 0) {
      ttl_seconds = -2;
    } else {
      ttl_seconds = ttl;
    }
    appendInteger(res, ttl_seconds);
    write_callback_(res);
  } else {
    // 用"-3"标识一个单独的错误状态
    write_callback_(REDIS_RES_INTEGER_NEGATIVE_THREE);
  }
}

void RedisProtocol::pttlCmd(const std::string& key) {
  VLOG(5) << "Pttlcmd the key is:" << key;

  // Not register the function
  if (!laser_ttl_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_ttl_callback!!!";
    return;
  }

  int64_t pttl;
  auto ret = laser_ttl_callback_(&pttl, key);
  if (ret) {
    std::string res;
    if (pttl == 0) {
      pttl = -2;
    }
    appendInteger(res, pttl);
    write_callback_(res);
  } else {
    // 用"-3"标识一个单独的错误状态
    write_callback_(REDIS_RES_INTEGER_NEGATIVE_THREE);
  }
}
void RedisProtocol::existsCmd(const std::vector<std::string>& keys) {
  VLOG(5) << "Existscmd the redis first key is:" << keys.at(0);

  // Not register the function
  if (!laser_exists_callback_) {
    write_callback_(REDIS_RES_NULL);
    VLOG(3) << "Not register laser_exists_callback!!!";
    return;
  }

  uint32_t count = 0;
  auto ret = laser_exists_callback_(&count, keys);
  if (ret) {
    std::string res;
    appendInteger(res, count);
    write_callback_(res);
  } else {
    write_callback_(REDIS_RES_INTEGER_ZERO);
    VLOG(5) << "Exists cmd response is zero, the first key is:" << keys.at(0);
  }
}

}  // namespace laser
