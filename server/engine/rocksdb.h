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
 */

#pragma once

#include "folly/io/IOBuf.h"

#include "common/laser/laser_entity.h"
#include "common/laser/format.h"

#include "replication_db.h"

namespace laser {

inline constexpr char LASER_ROCKSDB_ENGINE_MODULE_NAME[] = "rocksdb_engine";

struct RocksDbEngineOptions {
  uint64_t ttl;
};

struct RocksDbEngineSetOptions {
  uint64_t ttl = 0;
  bool not_exists = false;  // 当等于 true 时只有 key 存在才会set
};

class RocksDbEngine : public std::enable_shared_from_this<RocksDbEngine> {
 public:
  explicit RocksDbEngine(std::shared_ptr<ReplicationDB> db, std::shared_ptr<RocksDbEngineOptions> options = nullptr)
      : db_(db), options_(options) {
    if (!options_) {
      ttl_ = 0;
    } else {
      ttl_ = options_->ttl;
    }
  }

  virtual ~RocksDbEngine() = default;

  virtual bool open() { return db_ && db_->open(); }

  virtual bool close() { return db_ && db_->close(); }

  // common
  virtual Status delkey(const LaserKeyFormat& key);
  virtual Status expire(const LaserKeyFormat& key, uint64_t time);
  virtual Status expireAt(const LaserKeyFormat& key, uint64_t time_at);
  // 过期返回 0，没有设置过期时间返回 -1
  virtual Status ttl(int64_t* ttl, const LaserKeyFormat& key);

  // string
  virtual Status append(uint32_t* length, const LaserKeyFormat& key, const std::string& data);
  virtual Status set(const LaserKeyFormat& key, const std::string& data);
  virtual Status setx(const LaserKeyFormat& key, const std::string& data, const RocksDbEngineSetOptions& options);
  virtual Status mset(const std::vector<LaserKeyFormat>& keys, const std::vector<std::string>& datas);
  virtual Status msetx(const std::vector<LaserKeyFormat>& keys, const std::vector<std::string>& datas,
                       const RocksDbEngineSetOptions& options);
  virtual Status get(LaserValueRawString* value, const LaserKeyFormat& key);
  virtual Status exist(bool* result, const LaserKeyFormat& key);

  // counter
  virtual Status decr(int64_t* value, const LaserKeyFormat& key, uint64_t step = 1);
  virtual Status incr(int64_t* value, const LaserKeyFormat& key, uint64_t step = 1);

  // hash map
  virtual Status hset(const LaserKeyFormat& key, const std::string& field, const std::string& value);
  virtual Status hmset(const LaserKeyFormat& key, const std::map<std::string, std::string>& values);
  virtual Status hdel(const LaserKeyFormat& key, const std::string& field);
  virtual Status hget(LaserValueRawString* value, const LaserKeyFormat& key, const std::string& field);
  virtual Status hlen(LaserValueMapMeta* value, const LaserKeyFormat& key);
  virtual Status hkeys(std::vector<LaserKeyFormatMapData>* keys, const LaserKeyFormat& key);
  virtual Status hgetall(std::unordered_map<std::string, LaserValueRawString>* values, const LaserKeyFormat& key);

  // list
  virtual Status llen(LaserValueListMeta* value, const LaserKeyFormat& key);
  virtual Status lindex(LaserValueRawString* value, const LaserKeyFormat& key, int64_t index);
  virtual Status pushFront(const LaserKeyFormat& key, const std::string& value);
  virtual Status pushBack(const LaserKeyFormat& key, const std::string& value);
  virtual Status popFront(LaserValueRawString* value, const LaserKeyFormat& key);
  virtual Status popBack(LaserValueRawString* value, const LaserKeyFormat& key);
  virtual Status get(LaserValueListMeta* value, const LaserKeyFormat& key);
  virtual Status lrange(std::vector<LaserValueRawString>* values, const LaserKeyFormat& key, uint64_t start = 0,
                        uint64_t end = 0);

  // set
  virtual Status sadd(const LaserKeyFormat& key, const std::string& member);
  virtual Status hasMember(const LaserKeyFormat& key, const std::string& member);
  virtual Status get(LaserValueSetMeta* value, const LaserKeyFormat& key);
  virtual Status sdel(const LaserKeyFormat& key, const std::string& member);
  virtual Status members(std::vector<LaserKeyFormatSetData>* members, const LaserKeyFormat& key);

  // zset
  virtual Status zadd(const LaserKeyFormat& key, const std::map<std::string, int64_t>& member_scores);
  virtual Status zrangeByScore(std::vector<LaserScoreMember>* score_members, const LaserKeyFormat& key, int64_t min,
                               int64_t max);
  virtual Status zremRangeByScore(int64_t* number, const LaserKeyFormat& key, int64_t min, int64_t max);

  // db opt
  virtual Status ingestBaseSst(const std::string& ingest_file);
  virtual Status ingestDeltaSst(const std::string& ingest_file, const std::string& tempdb_path);
  virtual Status dumpSst(const std::string& sst_file_path);
  virtual std::weak_ptr<ReplicationDB> getReplicationDB() { return db_; }

 private:
  std::shared_ptr<ReplicationDB> db_;
  Status setCounterByStep(int64_t* result, const LaserKeyFormat& key, int64_t step);
  uint64_t ttl_;
  std::shared_ptr<RocksDbEngineOptions> options_;
  Status listPop(LaserValueRawString* value, const LaserKeyFormat& key, bool is_left);
  Status listPush(const LaserKeyFormat& key, const std::string& value, bool is_left);
  bool checkKeyExpire(const LaserValueFormatBase& value);
  void setAutoExpire(LaserValueFormatBase& value);                                         // NOLINT
  void setExpire(LaserValueFormatBase& value, uint64_t ttl);                               // NOLINT
  void setMapExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp);   // NOLINT
  void setListExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp);  // NOLINT
  void setSetExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp);   // NOLINT
  void zsetSetExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp);   // NOLINT
  void rangeZset(int64_t min, int64_t max, const LaserKeyFormat& key, rocksdb::Iterator* iter,
                 IteratorCallback callback);
};

}  // namespace laser
