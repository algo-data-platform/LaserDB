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


#include "boost/filesystem.hpp"
#include "common/util.h"

#include "scoped_key_lock.h"
#include "rocksdb.h"

namespace laser {

constexpr double LASER_ROCKSDB_ENGINE_TIMER_BUCKET_SIZE = 1.0;
constexpr double LASER_ROCKSDB_ENGINE_TIMER_MIN = 0.0;
constexpr double LASER_ROCKSDB_ENGINE_TIMER_MAX = 1000.0;

Status RocksDbEngine::append(uint32_t* length, const LaserKeyFormat& key, const std::string& data) {
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueRawString old_value;
  Status status = db_->read(&old_value, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  RocksDbBatch batch;
  std::string set_data;
  if (status == Status::OK) {
    if (!old_value.decode()) {
      return Status::RS_INVALID_ARGUMENT;
    }
    set_data = old_value.getValue() + data;
  } else {
    set_data = data;
  }

  *length = set_data.size();
  LaserValueRawString value(set_data);
  setAutoExpire(value);
  value.encode();
  batch.iput(key, value);
  return db_->write(batch);
}

Status RocksDbEngine::set(const LaserKeyFormat& key, const std::string& data) {
  RocksDbBatch batch;
  LaserValueRawString value(data);
  setAutoExpire(value);
  value.encode();
  batch.iput(key, value);
  return db_->write(batch);
}

Status RocksDbEngine::setx(const LaserKeyFormat& key, const std::string& data, const RocksDbEngineSetOptions& options) {
  ScopedKeyLock guard(std::string(key.data(), key.length()));
  RocksDbBatch batch;

  LaserValueRawString value(data);
  if (options.not_exists) {
    LaserValueRawString old_value;
    Status status = db_->read(&old_value, key);
    if (status != Status::OK && status != Status::RS_NOT_FOUND) {
      return status;
    }

    if (status == Status::OK) {
      if (!old_value.decode()) {
        return Status::RS_INVALID_ARGUMENT;
      }
      // 如果存在并且没有过期则不设置
      if (!checkKeyExpire(old_value)) {
        return Status::RS_KEY_EXISTS;
      }
    }
  }

  if (options.ttl > 0) {
    setExpire(value, options.ttl);
  } else {
    setAutoExpire(value);
  }
  value.encode();
  batch.iput(key, value);
  return db_->write(batch);
}

Status RocksDbEngine::exist(bool* result, const LaserKeyFormat& key) {
  bool ret;
  LaserValueRawString laser_value;
  auto status = db_->exist(&ret, laser_value.getRawBuffer(), key);
  if (!ret) {
    *result = false;
    return status;
  }

  if (!laser_value.decode()) {
    *result = false;
    return Status::RS_INVALID_ARGUMENT;
  }
  // value expire return false
  if (checkKeyExpire(laser_value)) {
    *result = false;
  } else {
    *result = true;
  }
  return status;
}

Status RocksDbEngine::mset(const std::vector<LaserKeyFormat>& keys, const std::vector<std::string>& datas) {
  if (keys.size() != datas.size()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  RocksDbBatch batch;

  for (size_t i = 0; i < keys.size(); i++) {
    LaserValueRawString value(datas[i]);
    setAutoExpire(value);
    value.encode();
    batch.iput(keys[i], value);
  }
  return db_->write(batch);
}

Status RocksDbEngine::msetx(const std::vector<LaserKeyFormat>& keys, const std::vector<std::string>& datas,
                            const RocksDbEngineSetOptions& options) {
  if (keys.size() != datas.size()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  RocksDbBatch batch;
  std::vector<ScopedKeyLock> key_locks;
  for (size_t i = 0; i < keys.size(); ++i) {
    key_locks.emplace_back(std::move(ScopedKeyLock(std::string(keys[i].data(), keys[i].length()))));
    LaserValueRawString value(datas[i]);
    if (options.not_exists) {
      LaserValueRawString old_value;
      Status status = db_->read(&old_value, keys[i]);
      if (status != Status::OK && status != Status::RS_NOT_FOUND) {
        continue;
      }

      if (status == Status::OK) {
        if (!old_value.decode()) {
          continue;
        }
        if (!checkKeyExpire(old_value)) {
          continue;
        }
      }
    }

    if (options.ttl > 0) {
      setExpire(value, options.ttl);
    } else {
      setAutoExpire(value);
    }
    value.encode();
    batch.iput(keys[i], value);
  }
  return db_->write(batch);
}

Status RocksDbEngine::get(LaserValueRawString* value, const LaserKeyFormat& key) {
  Status status = db_->read(value, key);
  if (status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  if (checkKeyExpire(*value)) {
    return Status::RS_KEY_EXPIRE;
  }
  return status;
}

Status RocksDbEngine::setCounterByStep(int64_t* result, const LaserKeyFormat& key, int64_t step) {
  ScopedKeyLock guard(std::string(key.data(), key.length()));
  RocksDbBatch batch;
  LaserValueCounter old_counter;
  Status status = db_->read(&old_counter, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  int64_t count = 0;
  if (status == Status::OK) {
    if (!old_counter.decode()) {
      return Status::RS_INVALID_ARGUMENT;
    }
    count = old_counter.getValue();
  }
  count += step;

  *result = count;
  LaserValueCounter value(count);
  setAutoExpire(value);
  value.encode();
  batch.iput(key, value);
  return db_->write(batch);
}

Status RocksDbEngine::incr(int64_t* value, const LaserKeyFormat& key, uint64_t step) {
  return setCounterByStep(value, key, step);
}

Status RocksDbEngine::decr(int64_t* value, const LaserKeyFormat& key, uint64_t step) {
  return setCounterByStep(value, key, -step);
}

Status RocksDbEngine::hset(const LaserKeyFormat& key, const std::string& field, const std::string& value) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueMapMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  LaserValueRawString field_value(value);
  LaserKeyFormatMapData map_data_key(key, field);
  if (status == Status::OK) {
    // 获取数据
    LaserValueRawString old_value;
    Status data_status = db_->read(&old_value, map_data_key);
    if (data_status != Status::OK && data_status != Status::RS_NOT_FOUND) {
      return data_status;
    }

    if (!meta_data.decode()) {
      return Status::RS_INVALID_ARGUMENT;
    }
    if (data_status == Status::RS_NOT_FOUND) {
      meta_data.incrSize();
      meta_data.encode();
      setAutoExpire(meta_data);
      batch.iput(key, meta_data);
    }
    setAutoExpire(field_value);
    batch.iput(map_data_key, field_value);
  } else {
    meta_data.incrSize();
    setAutoExpire(meta_data);
    meta_data.encode();
    batch.iput(key, meta_data);
    batch.iput(map_data_key, field_value);
  }

  return db_->write(batch);
}

Status RocksDbEngine::hmset(const LaserKeyFormat& key, const std::map<std::string, std::string>& values) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueMapMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  if (status == Status::OK) {
    if (!meta_data.decode()) {
      return Status::RS_INVALID_ARGUMENT;
    }
  }

  setAutoExpire(meta_data);
  for (auto& value : values) {
    LaserValueRawString field_value(value.second);
    setAutoExpire(field_value);
    LaserKeyFormatMapData map_data_key(key, value.first);
    if (status == Status::OK) {
      // 获取数据
      LaserValueRawString old_value;
      Status data_status = db_->read(&old_value, map_data_key);
      if (data_status != Status::OK && data_status != Status::RS_NOT_FOUND) {
        return data_status;
      }

      if (data_status == Status::RS_NOT_FOUND) {
        meta_data.incrSize();
        meta_data.encode();
        batch.iput(key, meta_data);
      }
      batch.iput(map_data_key, field_value);
    } else {
      meta_data.incrSize();
      meta_data.encode();
      batch.iput(key, meta_data);
      batch.iput(map_data_key, field_value);
    }
  }

  return db_->write(batch);
}

Status RocksDbEngine::hdel(const LaserKeyFormat& key, const std::string& field) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueMapMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK) {
    return status;
  }
  if (!meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  LaserKeyFormatMapData map_data_key(key, field);
  // 获取数据
  LaserValueRawString old_value;
  Status data_status = db_->read(&old_value, map_data_key);
  if (data_status != Status::OK) {
    return data_status;
  }

  batch.idelete(map_data_key);
  meta_data.decrSize();
  meta_data.encode();
  batch.iput(key, meta_data);

  return db_->write(batch);
}

Status RocksDbEngine::hget(LaserValueRawString* value, const LaserKeyFormat& key, const std::string& field) {
  LaserKeyFormatMapData map_data_key(key, field);

  LaserValueMapMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK) {
    return status;
  }

  if (!meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(meta_data)) {
    return Status::RS_KEY_EXPIRE;
  }

  Status data_status = db_->read(value, map_data_key);
  if (data_status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  return data_status;
}

Status RocksDbEngine::hlen(LaserValueMapMeta* value, const LaserKeyFormat& key) {
  Status status = db_->read(value, key);
  if (status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(*value)) {
    return Status::RS_KEY_EXPIRE;
  }

  return status;
}

Status RocksDbEngine::hkeys(std::vector<LaserKeyFormatMapData>* keys, const LaserKeyFormat& key) {
  LaserValueMapMeta map_meta;
  Status status = db_->read(&map_meta, key);
  if (status != Status::OK) {
    return status;
  }
  if (!map_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(map_meta)) {
    return Status::RS_KEY_EXPIRE;
  }

  db_->iterator([this, &key, keys, &status](auto iter) {
    LaserKeyFormatMapData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatMapData map_data_key(iter->key().data(), iter->key().size());
      if (!map_data_key.decode()) {
        status = Status::RS_INVALID_ARGUMENT;
        return;
      }
      keys->push_back(map_data_key);
    }
  });

  return status;
}

Status RocksDbEngine::hgetall(std::unordered_map<std::string, LaserValueRawString>* values, const LaserKeyFormat& key) {
  LaserValueMapMeta map_meta;
  Status status = db_->read(&map_meta, key);
  if (status != Status::OK) {
    return status;
  }
  if (!map_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(map_meta)) {
    return Status::RS_KEY_EXPIRE;
  }

  db_->iterator([this, &key, values](auto iter) {
    LaserKeyFormatMapData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatMapData map_data_key(iter->key().data(), iter->key().size());
      if (!map_data_key.decode()) {
        continue;
      }
      LaserValueRawString map_value(iter->value().data(), iter->value().size());
      if (!map_value.decode()) {
        continue;
      }

      (*values)[map_data_key.getField()] = map_value;
    }
  });

  return status;
}

Status RocksDbEngine::llen(LaserValueListMeta* value, const LaserKeyFormat& key) {
  Status status = db_->read(value, key);
  if (status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(*value)) {
    return Status::RS_KEY_EXPIRE;
  }

  return status;
}

Status RocksDbEngine::lindex(LaserValueRawString* value, const LaserKeyFormat& key, int64_t index) {
  LaserValueListMeta list_meta;
  Status status = db_->read(&list_meta, key);
  if (status != Status::OK) {
    return status;
  }

  if (!list_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(list_meta)) {
    return Status::RS_KEY_EXPIRE;
  }
  int64_t target_index = index >= 0 ? (list_meta.getStart() + index + 1) : (list_meta.getEnd() + index);
  LaserKeyFormatListData list_data_key(key, target_index);
  status = db_->read(value, list_data_key);
  if (status != Status::OK) {
    return status;
  }
  if (!value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  return Status::OK;
}

Status RocksDbEngine::listPop(LaserValueRawString* value, const LaserKeyFormat& key, bool is_left) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueListMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK) {
    return status;
  }

  if (!meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(meta_data)) {
    return Status::RS_KEY_EXPIRE;
  }
  if (meta_data.getSize() == 0) {
    return Status::RS_EMPTY;
  }
  int64_t index = is_left ? meta_data.popFront() : meta_data.popBack();
  meta_data.encode();
  batch.iput(key, meta_data);

  LaserKeyFormatListData list_data_key(key, index);
  status = db_->read(value, list_data_key);
  if (status != Status::OK) {
    return status;
  }
  if (!value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  batch.idelete(list_data_key);
  return db_->write(batch);
}

Status RocksDbEngine::popFront(LaserValueRawString* value, const LaserKeyFormat& key) {
  return listPop(value, key, true);
}

Status RocksDbEngine::popBack(LaserValueRawString* value, const LaserKeyFormat& key) {
  return listPop(value, key, false);
}

Status RocksDbEngine::get(LaserValueListMeta* value, const LaserKeyFormat& key) {
  Status status = db_->read(value, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }
  if (status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(*value)) {
    return Status::RS_KEY_EXPIRE;
  }

  return Status::OK;
}

Status RocksDbEngine::lrange(std::vector<LaserValueRawString>* values, const LaserKeyFormat& key, uint64_t start,
                             uint64_t end) {
  LaserValueListMeta list_meta;
  Status status = db_->read(&list_meta, key);
  if (status != Status::OK) {
    return status;
  }

  if (!list_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(list_meta)) {
    return Status::RS_KEY_EXPIRE;
  }
  int64_t target_index_start = list_meta.getStart() + start + 1;
  int64_t target_index_end = list_meta.getEnd() - 1;

  if (end > 0) {  // 用户指定 start end index
    if (end <= start) {
      return Status::RS_INVALID_ARGUMENT;
    }
    target_index_end = list_meta.getStart() + end;
  }
  if (target_index_start > list_meta.getEnd() || target_index_end > list_meta.getEnd()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  db_->iterator([this, &key, values, target_index_start, target_index_end](auto iter) {
    LaserKeyFormatListData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatListData list_data_key(iter->key().data(), iter->key().size());
      if (!list_data_key.decode()) {
        continue;
      }
      int64_t index = list_data_key.getIndex();
      if (index < target_index_start) {
        continue;
      }

      if (index > target_index_end) {
        continue;
      }
      LaserValueRawString list_value(iter->value().data(), iter->value().size());
      if (!list_value.decode()) {
        continue;
      }
      values->push_back(list_value);
    }
  });

  return status;
}

Status RocksDbEngine::listPush(const LaserKeyFormat& key, const std::string& value, bool is_left) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueListMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }
  if (status == Status::OK && !meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  setAutoExpire(meta_data);

  int64_t index = is_left ? meta_data.pushFront() : meta_data.pushBack();
  meta_data.encode();
  batch.iput(key, meta_data);

  LaserValueRawString item_value(value);
  LaserKeyFormatListData list_data_key(key, index);

  setAutoExpire(item_value);
  batch.iput(list_data_key, item_value);
  return db_->write(batch);
}

Status RocksDbEngine::pushFront(const LaserKeyFormat& key, const std::string& value) {
  return listPush(key, value, true);
}

Status RocksDbEngine::pushBack(const LaserKeyFormat& key, const std::string& value) {
  return listPush(key, value, false);
}

Status RocksDbEngine::sadd(const LaserKeyFormat& key, const std::string& member) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueSetMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  LaserKeyFormatSetData set_data_member(key, member);
  LaserValueRawString null_value;
  if (status == Status::OK) {
    // 获取数据
    Status data_status = db_->read(&null_value, set_data_member);
    if (data_status != Status::OK && data_status != Status::RS_NOT_FOUND) {
      return data_status;
    }

    if (!meta_data.decode()) {
      return Status::RS_INVALID_ARGUMENT;
    }
    if (data_status == Status::RS_NOT_FOUND) {
      meta_data.incrSize();
      meta_data.encode();
      batch.iput(key, meta_data);
      setAutoExpire(null_value);
      batch.iput(set_data_member, null_value);
    }
  } else {
    meta_data.incrSize();
    setAutoExpire(meta_data);
    meta_data.encode();
    batch.iput(key, meta_data);
    setAutoExpire(null_value);
    batch.iput(set_data_member, null_value);
  }

  return db_->write(batch);
}

Status RocksDbEngine::hasMember(const LaserKeyFormat& key, const std::string& member) {
  LaserValueSetMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK) {
    return status;
  }
  if (!meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(meta_data)) {
    return Status::RS_KEY_EXPIRE;
  }

  LaserKeyFormatSetData set_data_member(key, member);
  LaserValueRawString null_value;
  return db_->read(&null_value, set_data_member);
}

Status RocksDbEngine::get(LaserValueSetMeta* value, const LaserKeyFormat& key) {
  Status status = db_->read(value, key);
  if (status == Status::OK && !value->decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  if (checkKeyExpire(*value)) {
    return Status::RS_KEY_EXPIRE;
  }
  return status;
}

Status RocksDbEngine::sdel(const LaserKeyFormat& key, const std::string& member) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueSetMeta meta_data;
  Status status = db_->read(&meta_data, key);
  if (status != Status::OK) {
    return status;
  }

  LaserKeyFormatSetData set_data_member(key, member);
  LaserValueRawString null_value;
  Status data_status = db_->read(&null_value, set_data_member);
  if (data_status != Status::OK) {
    return data_status;
  }

  if (!meta_data.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  meta_data.decrSize();
  meta_data.encode();
  batch.iput(key, meta_data);
  batch.idelete(set_data_member);

  return db_->write(batch);
}

Status RocksDbEngine::members(std::vector<LaserKeyFormatSetData>* members, const LaserKeyFormat& key) {
  LaserValueSetMeta set_meta;
  Status status = db_->read(&set_meta, key);
  if (status != Status::OK) {
    return status;
  }
  if (!set_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }
  if (checkKeyExpire(set_meta)) {
    return Status::RS_KEY_EXPIRE;
  }

  db_->iterator([this, &key, members](auto iter) {
    LaserKeyFormatSetData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatSetData set_data_member(iter->key().data(), iter->key().size());
      if (!set_data_member.decode()) {
        continue;
      }
      members->push_back(set_data_member);
    }
  });

  return status;
}

// !注意ZSet目前只支持key_socre维度的唯一，不支持key_memebr的唯一性,zset_meta中size为key_score的个数
Status RocksDbEngine::zadd(const LaserKeyFormat& key, const std::map<std::string, int64_t>& member_scores) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueZSetMeta zset_meta;
  Status status = db_->read(&zset_meta, key);
  if (status != Status::OK && status != Status::RS_NOT_FOUND) {
    return status;
  }

  for (auto& member_score : member_scores) {
    LaserValueZSet members_value;
    LaserKeyFormatZSetData zset_data_key(key, member_score.second);
    if (status == Status::OK) {
      // 读取members
      Status data_status = db_->read(&members_value, zset_data_key);
      if (data_status != Status::OK && data_status != Status::RS_NOT_FOUND) {
        return data_status;
      }

      if (!zset_meta.decode()) {
        return Status::RS_INVALID_ARGUMENT;
      }

      if (data_status == Status::OK) {
        members_value.decode();
        auto members = members_value.getMembers();
        bool member_exist = false;
        for (auto& member : members) {
          if (!member.compare(member_score.first)) {
            member_exist = true;
            break;
          }
        }
        if (member_exist) {
          // update the expire time
          setAutoExpire(zset_meta);
          zset_meta.encode();
          batch.iput(key, zset_meta);

          continue;
        }

        members_value.addMember(member_score.first);
      }
    }
    // when members_value not contains any member, then it is a new key_score/member
    if (members_value.getMembers().size() == 0) {
      members_value.addMember(member_score.first);
      zset_meta.incrSize();
    }

    // update the zset_meta
    setAutoExpire(zset_meta);
    zset_meta.encode();
    batch.iput(key, zset_meta);

    // set members
    setAutoExpire(members_value);
    members_value.encode();
    batch.iput(zset_data_key, members_value);
  }

  return db_->write(batch);
}

Status RocksDbEngine::zrangeByScore(std::vector<LaserScoreMember>* score_members, const LaserKeyFormat& key,
                                    int64_t min, int64_t max) {
  if (min > max) {
    return Status::OK;
  }

  LaserValueZSetMeta zset_meta;
  Status status = db_->read(&zset_meta, key);
  if (status != Status::OK) {
    return status;
  }

  if (!zset_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  if (checkKeyExpire(zset_meta)) {
    return Status::RS_KEY_EXPIRE;
  }

  db_->iterator([this, &min, &max, &key, score_members](auto iter) {
    this->rangeZset(min, max, key, iter, [score_members](auto iter) {
      LaserKeyFormatZSetData zset_data_key(iter->key().data(), iter->key().size());
      if (!zset_data_key.decode()) {
        return;
      }

      LaserValueZSet zset_value(iter->value().data(), iter->value().size());
      if (!zset_value.decode()) {
        return;
      }

      LaserScoreMember score_member;
      auto score = zset_data_key.getScore();
      score_member.set_score(score);
      auto zset_members = zset_value.getMembers();
      for (auto member : zset_members) {
        score_member.set_member(member);
        score_members->emplace_back(score_member);
      }
    });
  });
  return status;
}

Status RocksDbEngine::zremRangeByScore(int64_t* number, const LaserKeyFormat& key, int64_t min, int64_t max) {
  *number = 0;
  if (min > max) {
    return Status::OK;
  }

  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueZSetMeta zset_meta;
  Status status = db_->read(&zset_meta, key);
  if (status != Status::OK) {
    return status;
  }

  if (!zset_meta.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  if (checkKeyExpire(zset_meta)) {
    return Status::RS_KEY_EXPIRE;
  }

  db_->iterator([this, &key, &min, &max, &number, &batch, &zset_meta](auto iter) {
    this->rangeZset(min, max, key, iter, [&number, &batch, &zset_meta](auto iter) {
      LaserKeyFormatZSetData zset_data_key(iter->key().data(), iter->key().size());
      if (!zset_data_key.decode()) {
        return;
      }
      batch.idelete(zset_data_key);
      zset_meta.decrSize();
      (*number)++;
    });
  });

  // key下所有关联key_score全部删除
  if (zset_meta.getSize() == 0) {
    batch.idelete(key);
  } else {
    zset_meta.encode();
    batch.iput(key, zset_meta);
  }

  return db_->write(batch);
}

Status RocksDbEngine::delkey(const LaserKeyFormat& key) {
  ScopedKeyLock guard(std::string(key.data(), key.length()));
  return db_->delkey(key);
}

Status RocksDbEngine::expire(const LaserKeyFormat& key, uint64_t time) {
  uint64_t timestamp = time + static_cast<uint64_t>(common::currentTimeInMs());
  return expireAt(key, timestamp);
}

Status RocksDbEngine::expireAt(const LaserKeyFormat& key, uint64_t timestamp) {
  RocksDbBatch batch;
  ScopedKeyLock guard(std::string(key.data(), key.length()));

  LaserValueFormatBase value;
  Status status = db_->read(&value, key);
  if (status != Status::OK) {
    return status;
  }
  if (!value.decode()) {
    return Status::RS_INVALID_ARGUMENT;
  }

  // 设置 value 的 timestamp
  switch (value.getType()) {
    case ValueType::RAW_STRING: {
      LaserValueRawString string_value(value.data(), value.length());
      string_value.decode();
      string_value.setTimestamp(timestamp);
      string_value.encode();
      batch.iput(key, string_value);
      break;
    }
    case ValueType::COUNTER: {
      LaserValueCounter counter(value.data(), value.length());
      counter.decode();
      counter.setTimestamp(timestamp);
      counter.encode();
      batch.iput(key, counter);
      break;
    }
    case ValueType::MAP: {
      LaserValueMapMeta map(value.data(), value.length());
      map.decode();
      map.setTimestamp(timestamp);
      map.encode();
      batch.iput(key, map);
      setMapExpire(batch, key, timestamp);
      break;
    }
    case ValueType::LIST: {
      LaserValueListMeta list(value.data(), value.length());
      list.decode();
      list.setTimestamp(timestamp);
      list.encode();
      batch.iput(key, list);
      setListExpire(batch, key, timestamp);
      break;
    }
    case ValueType::SET: {
      LaserValueSetMeta set(value.data(), value.length());
      set.decode();
      set.setTimestamp(timestamp);
      set.encode();
      batch.iput(key, set);
      setSetExpire(batch, key, timestamp);
      break;
    }
    case ValueType::ZSET: {
      LaserValueZSetMeta zset(value.data(), value.length());
      zset.decode();
      zset.setTimestamp(timestamp);
      zset.encode();
      batch.iput(key, zset);
      zsetSetExpire(batch, key, timestamp);
      break;
    }
    default:
      LOG(ERROR) << "Invalid value type.";
  }
  return db_->write(batch);
}

Status RocksDbEngine::ttl(int64_t* ttl, const LaserKeyFormat& key) {
  LaserValueFormatBase value;
  Status status = db_->read(&value, key);
  if (status == Status::RS_NOT_FOUND) {
    *ttl = -2;
    return Status::OK;
  }

  if (status != Status::OK) {
    return status;
  }

  value.decode();
  int64_t diff_time = value.getTimestamp() - static_cast<uint64_t>(common::currentTimeInMs());
  *ttl = (diff_time < 0) ? 0 : diff_time;
  if (value.getTimestamp() == 0) {  // 没有设置过期时间，返回 -1
    *ttl = -1;
  }
  return Status::OK;
}

Status RocksDbEngine::ingestBaseSst(const std::string& ingest_file) { return db_->ingestBaseSst(ingest_file); }

Status RocksDbEngine::ingestDeltaSst(const std::string& ingest_file, const std::string& tempdb_path) {
  return db_->ingestDeltaSst(ingest_file, tempdb_path);
}

Status RocksDbEngine::dumpSst(const std::string& sst_file_path) { return db_->dumpSst(sst_file_path); }

void RocksDbEngine::setAutoExpire(LaserValueFormatBase& value) {
  if (ttl_ == 0) {
    return;
  }
  setExpire(value, ttl_);
}

void RocksDbEngine::setExpire(LaserValueFormatBase& value, uint64_t ttl) {
  if (ttl == 0) {
    return;
  }
  uint64_t timestamp = ttl + static_cast<uint64_t>(common::currentTimeInMs());
  value.setTimestamp(timestamp);
}

bool RocksDbEngine::checkKeyExpire(const LaserValueFormatBase& value) {
  uint64_t timestamp = value.getTimestamp();
  if (timestamp != 0 && timestamp < static_cast<uint64_t>(common::currentTimeInMs())) {
    return true;
  }

  return false;
}

void RocksDbEngine::setMapExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp) {
  db_->iterator([this, &key, &batch, timestamp](auto iter) {
    LaserKeyFormatMapData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatMapData map_data_key(iter->key().data(), iter->key().size());
      if (!map_data_key.decode()) {
        continue;
      }
      LaserValueRawString map_value(iter->value().data(), iter->value().size());
      if (!map_value.decode()) {
        continue;
      }
      map_value.setTimestamp(timestamp);
      map_value.encode();
      batch.iput(map_data_key, map_value);
    }
  });
}

void RocksDbEngine::setListExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp) {
  db_->iterator([this, &key, &batch, timestamp](auto iter) {
    LaserKeyFormatListData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatListData list_data_key(iter->key().data(), iter->key().size());
      if (!list_data_key.decode()) {
        continue;
      }
      LaserValueRawString list_value(iter->value().data(), iter->value().size());
      if (!list_value.decode()) {
        continue;
      }
      list_value.setTimestamp(timestamp);
      list_value.encode();
      batch.iput(list_data_key, list_value);
    }
  });
}

void RocksDbEngine::setSetExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp) {
  db_->iterator([this, &key, &batch, timestamp](auto iter) {
    LaserKeyFormatSetData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatSetData set_data_key(iter->key().data(), iter->key().size());
      if (!set_data_key.decode()) {
        continue;
      }
      LaserValueRawString set_value(iter->value().data(), iter->value().size());
      if (!set_value.decode()) {
        continue;
      }
      set_value.setTimestamp(timestamp);
      set_value.encode();
      batch.iput(set_data_key, set_value);
    }
  });
}

void RocksDbEngine::zsetSetExpire(RocksDbBatch& batch, const LaserKeyFormat& key, uint64_t timestamp) {
  db_->iterator([this, &key, &batch, timestamp](auto iter) {
    LaserKeyFormatZSetData prefix(key);
    rocksdb::Slice slice_key(prefix.data(), prefix.length());

    for (iter->Seek(slice_key); iter->Valid() && iter->key().starts_with(slice_key); iter->Next()) {
      LaserKeyFormatZSetData zset_data_key(iter->key().data(), iter->key().size());
      if (!zset_data_key.decode()) {
        continue;
      }
      LaserValueZSet zset_value(iter->value().data(), iter->value().size());
      if (!zset_value.decode()) {
        continue;
      }
      zset_value.setTimestamp(timestamp);
      zset_value.encode();
      batch.iput(zset_data_key, zset_value);
    }
  });
}

void rangeMemberScore(const rocksdb::Iterator& iter, int64_t score, std::vector<LaserScoreMember>* score_members) {
  LaserValueZSet zset_value(iter.value().data(), iter.value().size());
  if (!zset_value.decode()) {
    return;
  }

  LaserScoreMember score_member;
  score_member.set_score(score);
  auto zset_members = zset_value.getMembers();
  for (auto member : zset_members) {
    score_member.set_member(member);
    score_members->emplace_back(score_member);
  }
}

void RocksDbEngine::rangeZset(int64_t min, int64_t max, const LaserKeyFormat& key, rocksdb::Iterator* iter,
                              IteratorCallback callback) {
  LaserKeyFormatZSetData zset_data_key_min(key, min);
  LaserKeyFormatZSetData zset_data_key_max(key, max);
  LaserKeyFormatZSetData zset_data_key_zero(key, 0);
  rocksdb::Slice slice_key_min(zset_data_key_min.data(), zset_data_key_min.length());
  rocksdb::Slice slice_key_max(zset_data_key_max.data(), zset_data_key_max.length());
  rocksdb::Slice slice_key_zero(zset_data_key_zero.data(), zset_data_key_zero.length());

  if (max < 0) {
    for (iter->Seek(slice_key_min);
         iter->Valid() && iter->key().compare(slice_key_min) >= 0 && iter->key().compare(slice_key_max) <= 0;
         iter->Next()) {
      callback(iter);
    }
  } else if (min < 0 && max >= 0) {
    for (iter->Seek(slice_key_min); iter->Valid() && iter->key().compare(slice_key_min) >= 0; iter->Next()) {
      callback(iter);
    }

    for (iter->Seek(slice_key_zero);
         iter->Valid() && iter->key().compare(slice_key_zero) >= 0 && iter->key().compare(slice_key_max) <= 0;
         iter->Next()) {
      callback(iter);
    }
  } else {
    for (iter->Seek(slice_key_min);
         iter->Valid() && iter->key().compare(slice_key_min) >= 0 && iter->key().compare(slice_key_max) <= 0;
         iter->Next()) {
      callback(iter);
    }
  }
}

}  // namespace laser
