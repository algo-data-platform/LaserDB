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

namespace cpp2 laser

struct LaserKey {
  1: required string database_name
  2: required string table_name
  3: required list<string> primary_keys
  4: list<string> column_keys
}

struct LaserKeys {
  1: required list<LaserKey> keys
}

struct EntryValue {
  1: required Status status
  2: required string string_value
}

union LaserValue {
  1: string string_value
  2: map<string, string> map_value
  3: bool null_value
  4: map<string, i64> member_score_value
  5: EntryValue entry_value
}

struct LaserKV {
  1: required LaserKey key
  2: required LaserValue value 
}

struct LaserSetOption {
  1: required bool not_exists  
  2: required i64 ttl  
}

struct LaserKVs {
  1: list<LaserKV> values
}

struct LaserScoreMember {
  1: required i64 score
  2: required string member
}

union LaserResponse {
  1: i64 int_data
  2: string string_data
  3: list<i64> list_int_data
  4: list<string> list_string_data
  5: list<LaserValue> list_value_data
  6: map<string, string> map_string_data
  7: map<string, LaserValue> map_value_data
  8: bool bool_data
  9: list<LaserScoreMember> list_score_member_data
}

enum Status {
  OK = 0,
  
  RS_NOT_FOUND = 1,
  RS_CORRUPTION = 2,
  RS_NOT_SUPPORTED = 3,
  RS_INVALID_ARGUMENT = 4,
  RS_IO_ERROR = 5,
  RS_MERGE_INPROGRESS = 6,
  RS_IN_COMPLETE = 7,
  RS_SHUTDOWN_INPROGRESS = 8,
  RS_TIMEDOUT = 9,
  RS_ABORTED = 10,
  RS_BUSY = 11,
  RS_EXPIRED = 12,
  RS_TRYAGAIN = 13,
  RS_COMPACTION_TOO_LARGE = 14,
  RS_ERROR = 15,
  RS_EMPTY = 16,
  RS_WRITE_IN_FOLLOWER = 17,
  RS_KEY_EXPIRE = 18,
  RS_KEY_EXISTS = 19,
  RS_PART_FAILED = 20,
  RS_TRAFFIC_RESTRICTION = 21,
  RS_OPERATION_DENIED = 22,

  SERVICE_NOT_EXISTS_PARTITION = 101,
  SERVICE_UNION_DATA_TYPE_INVALID = 102,

  CLIENT_THRIFT_CALL_ERROR = 200,
  CLIENT_THRIFT_CALL_NO_SHARD_ID = 201,
  CLIENT_UNION_DATA_TYPE_INVALID = 202,
  CLIENT_THRIFT_CALL_TIMEOUT = 203,
  CLIENT_THRIFT_FUTURE_TIMEOUT = 204,

  RP_SOURCE_NOT_FOUND = 301,
  RP_ROLE_ERROR = 302,
  RP_SOURCE_READ_ERROR = 303,
  RP_SOURCE_DB_REMOVED = 304,
  RP_SOURCE_WAL_LOG_REMOVED = 305,

  GENERATOR_TABLE_NOT_EXISTS = 401,
  GENERATOR_GET_TABLE_LOCK_FAIL = 402,
  GENERATOR_TABLE_PROCESSING = 403,
  GENERATOR_TABLE_SET_QUEUE_FAIL = 404,
  GENERATOR_TABLE_SET_HASH_FAIL = 405,
  GENERATOR_TABLE_SET_LOCK_FAIL = 406,
  GENERATOR_TABLE_DEL_LOCK_FAIL = 407,
  GENERATOR_TABLE_DEL_QUEUE_FAIL = 408,

  UNKNOWN_ERROR = 10000,
}

exception LaserException {
  1: required string message,
  2: required Status status,
}

service LaserService {
  // key 相关操作 支持删除 key
  LaserResponse delkey(1: LaserKey key) throws (1: LaserException e)
  // 如果存在返回 OK, 否则返回 NOT_EXISTS
  LaserResponse exists(1: LaserKey key) throws (1: LaserException e)
  // 设置某个 key 的过期时间(相对时间)
  LaserResponse expire(1: LaserKey key, 2: i64 time) throws (1: LaserException e)
  // 设置某个 key 的过期时间(绝对时间)
  LaserResponse expireAt(1: LaserKey key, 2: i64 time_at) throws (1: LaserException e)
  // 查看某个 key 的 ttl 时间
  LaserResponse ttl(1: LaserKey key) throws (1: LaserException e)

  // raw string
  LaserResponse append(1: LaserKey key, 2: string value) throws (1: LaserException e)
  LaserResponse get(1: LaserKey key) throws (1: LaserException e)
  LaserResponse sset(1: LaserKV kv) throws (1: LaserException e)
  LaserResponse setx(1: LaserKV kv, 2: LaserSetOption option) throws (1: LaserException e)
  LaserResponse mget(1: LaserKeys keys) throws (1: LaserException e)
  LaserResponse mset(1: LaserKVs values) throws (1: LaserException e)
  LaserResponse mgetDetail(1: LaserKeys keys) throws (1: LaserException e)
  LaserResponse msetDetail(1: LaserKVs values, 2: LaserSetOption option) throws (1: LaserException e)
  LaserResponse mdel(1: LaserKeys keys) throws (1: LaserException e)
  LaserResponse exist(1: LaserKey key) throws (1: LaserException e)

  // counter
  LaserResponse decr(1: LaserKey key) throws (1: LaserException e)
  LaserResponse incr(1: LaserKey key) throws (1: LaserException e)
  LaserResponse decrBy(1: LaserKey key, 2: i64 step) throws (1: LaserException e)
  LaserResponse incrBy(1: LaserKey key, 2: i64 step) throws (1: LaserException e)

  // map
  LaserResponse hdel(1: LaserKey key, 2: string field) throws (1: LaserException e)
  LaserResponse hexists(1: LaserKey key, 2: string field) throws (1: LaserException e)
  LaserResponse hget(1: LaserKey key, 2: string field) throws (1: LaserException e)
  LaserResponse hset(1: LaserKey key, 2: string field, 3: string value) throws (1: LaserException e)
  LaserResponse hgetall(1: LaserKey key) throws (1: LaserException e)
  LaserResponse hkeys(1: LaserKey key) throws (1: LaserException e)
  LaserResponse hlen(1: LaserKey key) throws (1: LaserException e)
  LaserResponse hmget(1: LaserKey key, 2: list<string> fields) throws (1: LaserException e)
  LaserResponse hmset(1: LaserKey key, 2: LaserValue value) throws (1: LaserException e)

  // list
  LaserResponse lindex(1: LaserKey key, 2: i32 index) throws (1: LaserException e)
  LaserResponse llen(1: LaserKey key) throws (1: LaserException e)
  LaserResponse lpop(1: LaserKey key) throws (1: LaserException e)
  LaserResponse lpush(1: LaserKey key, 2: string value) throws (1: LaserException e)
  LaserResponse lrange(1: LaserKey key, 2: i32 start, 3: i32 end) throws (1: LaserException e)
  LaserResponse rpop(1: LaserKey key) throws (1: LaserException e)
  LaserResponse rpush(1: LaserKey key, 2: string value) throws (1: LaserException e)

  // set
  LaserResponse sadd(1: LaserKey key, 2: string member) throws (1: LaserException e)
  // 返回元素个数
  LaserResponse scard(1: LaserKey req) throws (1: LaserException e)
  LaserResponse sismember(1: LaserKey req, 2: string member) throws (1: LaserException e)
  LaserResponse sremove(1: LaserKey req, 2: string member) throws (1: LaserException e)
  LaserResponse smembers(1: LaserKey req) throws (1: LaserException e)
  
  // zset
  LaserResponse zadd(1: LaserKey key, 2: LaserValue member_score) throws (1: LaserException e)
  // 返回元素个数
  LaserResponse zcard(1: LaserKey key) throws (1: LaserException e)
  LaserResponse zrank(1: LaserKey key, 2: string member) throws (1: LaserException e)
  LaserResponse zscore(1: LaserKey key, 2: string member) throws (1: LaserException e)
  LaserResponse zrem(1: LaserKey key, 2: string member) throws (1: LaserException e)
  LaserResponse zrange(1: LaserKey key, 2: i64 start, 3: i64 stop) throws (1: LaserException e)
  LaserResponse zrangeByScore(1: LaserKey key, 2: i64 min, 3: i64 max) throws (1: LaserException e)
  LaserResponse zremRangeByScore(1: LaserKey key, 2: i64 min, 3: i64 max) throws (1: LaserException e)
}
