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

#include "folly/io/IOBuf.h"
#include "folly/lang/Bits.h"
#include "folly/Optional.h"
#include "city.h"

namespace laser {

enum class KeyType {
  DEFAULT = 1,
  COMPOSITE = 2,
  TTL_SORT = 3,
};

class LaserSerializer {
 public:
  LaserSerializer() {}
  LaserSerializer(const char* buffer, size_t length) : raw_data_(buffer, length) {}
  explicit LaserSerializer(const std::string& data) : raw_data_(data) {}
  virtual ~LaserSerializer() {}
  inline std::string* getRawBuffer() { return &raw_data_; }
  inline const char* data() const { return raw_data_.data(); }
  inline size_t length() const { return raw_data_.length(); }
  virtual inline void clear() { raw_data_.clear(); }
  virtual inline void resetOffset() { offset_ = 0; }
  virtual inline void reset() {
    clear();
    resetOffset();
  }

 protected:
  void packString(const std::string& str);
  void packString(const char* data, size_t size);
  bool unpackString(std::string* result);

  // liujunpeng: patch the Endian::little bug.To be compatible with old data, add the use_litte_endian parameter
  template <typename T>
  void packInt(typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type value,
               bool use_litte_endian = true) {
    if (use_litte_endian == true) {
      value = folly::Endian::little(value);
    } else {
      value = folly::Endian::big(value);
    }
    raw_data_.append(reinterpret_cast<const char*>(&value), sizeof(T));
  }

  template <typename T>
  bool unpackInt(typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type* value,
                 bool use_litte_endian = true) {
    if (raw_data_.length() < (offset_ + sizeof(T))) {
      return false;
    }
    T result = 0;
    memcpy(&result, raw_data_.data() + offset_, sizeof(T));
    offset_ += sizeof(T);
    if (use_litte_endian == true) {
      *value = folly::Endian::little(result);
    } else {
      *value = folly::Endian::big(result);
    }

    return true;
  }
  const std::string toHexString(std::string tok = "");

 private:
  std::string raw_data_;
  uint32_t offset_{0};
};

class LaserKeyFormatTypePrefix : public LaserSerializer {
 public:
  explicit LaserKeyFormatTypePrefix(const KeyType& key_type) : key_type_(key_type) {
    packInt<uint8_t>(static_cast<uint8_t>(key_type_));
  }
  ~LaserKeyFormatTypePrefix() = default;

 private:
  KeyType key_type_{KeyType::DEFAULT};
};

class LaserKeyFormatBase : public LaserSerializer {
 public:
  LaserKeyFormatBase() {}
  LaserKeyFormatBase(const char* buffer, size_t length) : LaserSerializer(buffer, length) {}
  LaserKeyFormatBase(const std::vector<std::string>& primary_keys, const std::vector<std::string>& columns,
                     const KeyType& key_type)
      : primary_keys_(primary_keys), columns_(columns), key_type_(key_type) {}
  LaserKeyFormatBase(const LaserKeyFormatBase& key, const KeyType& key_type)
      : primary_keys_(key.getPrimaryKeys()), columns_(key.getColumnFamilies()), key_type_(key_type) {}

  inline const std::vector<std::string>& getPrimaryKeys() const { return primary_keys_; }
  inline const std::vector<std::string>& getColumnFamilies() const { return columns_; }
  inline const KeyType& getKeyType() const { return key_type_; }
  inline int64_t getKeyHash() const {
    int64_t result = 0;
    for (auto& t : primary_keys_) {
      result = CityHash64WithSeed(t.c_str(), t.size(), result);
    }
    return result;
  }
  virtual bool decode();
  virtual void encode();
  virtual ~LaserKeyFormatBase() = default;

 private:
  std::vector<std::string> primary_keys_;
  std::vector<std::string> columns_;
  KeyType key_type_{KeyType::DEFAULT};
};

class LaserKeyFormat : public LaserKeyFormatBase {
 public:
  LaserKeyFormat() {}
  LaserKeyFormat(const char* buffer, size_t length) : LaserKeyFormatBase(buffer, length) {}
  LaserKeyFormat(const std::vector<std::string>& primary_keys, const std::vector<std::string>& columns)
      : LaserKeyFormatBase(primary_keys, columns, KeyType::DEFAULT) {
    encode();
  }
  LaserKeyFormat(const LaserKeyFormatBase& key, const KeyType& type) : LaserKeyFormatBase(key, type) { encode(); }
  ~LaserKeyFormat() = default;
};

class LaserKeyFormatMapData : public LaserKeyFormatBase {
 public:
  LaserKeyFormatMapData() {}
  LaserKeyFormatMapData(const char* buffer, size_t length) : LaserKeyFormatBase(buffer, length) {}
  LaserKeyFormatMapData(const LaserKeyFormatBase& key, const std::string& field)
      : LaserKeyFormatBase(key, KeyType::COMPOSITE), field_(field) {
    encode();
  }

  explicit LaserKeyFormatMapData(const LaserKeyFormatBase& key) : LaserKeyFormatMapData(key, "") {}

  ~LaserKeyFormatMapData() = default;
  inline bool decode() override {
    if (!LaserKeyFormatBase::decode()) {
      return false;
    }
    // 允许 field 字段为空
    unpackString(&field_);
    return true;
  }
  inline void encode() override {
    LaserKeyFormatBase::encode();
    packString(field_);
  }
  inline const std::string& getField() const { return field_; }

 private:
  std::string field_{""};
};

class LaserKeyFormatZSetData : public LaserKeyFormatBase {
 public:
  LaserKeyFormatZSetData() {}
  LaserKeyFormatZSetData(const char* buffer, size_t length) : LaserKeyFormatBase(buffer, length) {}
  LaserKeyFormatZSetData(const LaserKeyFormatBase& key, folly::Optional<int64_t> score)
      : LaserKeyFormatBase(key, KeyType::COMPOSITE), score_(score) {
    encode();
  }

  explicit LaserKeyFormatZSetData(const LaserKeyFormatBase& key) : LaserKeyFormatZSetData(key, folly::none) {}
  ~LaserKeyFormatZSetData() = default;
  inline bool decode() override {
    if (!LaserKeyFormatBase::decode()) {
      return false;
    }

    int64_t score;
    if (unpackInt<int64_t>(&score, false)) {
      score_ = score;
    } else {
      return false;
    }
    return true;
  }
  inline void encode() override {
    LaserKeyFormatBase::encode();
    if (score_) {
      packInt<int64_t>(*score_, false);
    }
  }
  inline const int64_t getScore() const { return *score_; }

 private:
  folly::Optional<int64_t> score_{0};
};

class LaserKeyFormatTtl : public LaserSerializer {
 public:
  LaserKeyFormatTtl() {}
  LaserKeyFormatTtl(const char* buffer, size_t length) : LaserSerializer(buffer, length) {}
  LaserKeyFormatTtl(const LaserKeyFormat& key, uint64_t timestamp) : key_(key), timestamp_(timestamp) { encode(); }

  inline const LaserKeyFormatBase& getKey() const { return key_; }
  inline uint64_t getTimestamp() const { return timestamp_; }
  virtual bool decode();
  virtual void encode();
  inline void reset() { LaserSerializer::reset(); }
  inline virtual void clear() { LaserSerializer::clear(); }
  virtual ~LaserKeyFormatTtl() = default;

 private:
  LaserKeyFormat key_;
  uint64_t timestamp_ = 0;
  KeyType key_type_{KeyType::TTL_SORT};
};

class LaserKeyFormatSetData : public LaserKeyFormatBase {
 public:
  LaserKeyFormatSetData() {}
  LaserKeyFormatSetData(const char* buffer, size_t length) : LaserKeyFormatBase(buffer, length) {}
  LaserKeyFormatSetData(const LaserKeyFormatBase& key, const std::string& data)
      : LaserKeyFormatBase(key, KeyType::COMPOSITE), data_(data) {
    encode();
  }
  explicit LaserKeyFormatSetData(const LaserKeyFormatBase& key) : LaserKeyFormatSetData(key, "") {}

  ~LaserKeyFormatSetData() = default;
  inline bool decode() override {
    if (!LaserKeyFormatBase::decode()) {
      return false;
    }
    unpackString(&data_);
    return true;
  }
  inline void encode() override {
    LaserKeyFormatBase::encode();
    packString(data_);
  }
  inline const std::string& getData() const { return data_; }

 private:
  std::string data_;
};

class LaserKeyFormatListData : public LaserKeyFormatBase {
 public:
  LaserKeyFormatListData() {}
  LaserKeyFormatListData(const char* buffer, size_t length) : LaserKeyFormatBase(buffer, length) {}
  LaserKeyFormatListData(const LaserKeyFormatBase& key, folly::Optional<int64_t> index)
      : LaserKeyFormatBase(key, KeyType::COMPOSITE), index_(index) {
    encode();
  }
  explicit LaserKeyFormatListData(const LaserKeyFormatBase& key) : LaserKeyFormatListData(key, folly::none) {}

  ~LaserKeyFormatListData() = default;
  inline bool decode() override {
    if (!LaserKeyFormatBase::decode()) {
      return false;
    }

    int64_t index = 0;
    if (unpackInt<int64_t>(&index, false)) {
      index_ = index;
    }
    return true;
  }
  inline void encode() override {
    LaserKeyFormatBase::encode();
    if (index_) {
      packInt<int64_t>(*index_, false);
    }
  }
  inline const int64_t getIndex() const { return (index_) ? *index_ : 0; }

 private:
  folly::Optional<int64_t> index_{0};
};

enum class ValueType {
  RAW_STRING = 1,
  COUNTER = 2,
  MAP = 3,
  LIST = 4,
  SET = 5,
  ZSET = 6
};

class LaserValueFormatBase : public LaserSerializer {
 public:
  LaserValueFormatBase() {}
  LaserValueFormatBase(const char* buffer, size_t length) : LaserSerializer(buffer, length) {}
  explicit LaserValueFormatBase(const ValueType& type, uint64_t timestamp = 0);
  inline const ValueType& getType() const { return type_; }
  inline uint64_t getTimestamp() const { return timestamp_; }
  inline void setTimestamp(uint64_t timestamp) { timestamp_ = timestamp; }
  virtual ~LaserValueFormatBase() = default;
  virtual bool decode();
  virtual void encode();
  inline void reset() { LaserSerializer::reset(); }
  inline virtual void clear() { LaserSerializer::clear(); }

 private:
  ValueType type_;
  uint64_t timestamp_;
};

class LaserValueRawString : public LaserValueFormatBase {
 public:
  LaserValueRawString() : LaserValueFormatBase(ValueType::RAW_STRING) {}
  LaserValueRawString(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueRawString(const std::string& value) : LaserValueFormatBase(ValueType::RAW_STRING), value_(value) {
    encode();
  }
  inline const std::string& getValue() const { return value_; }
  bool decode() override;
  void encode() override;
  inline void reset() override {
    LaserValueFormatBase::reset();
    value_.clear();
  }

 private:
  std::string value_;
};

class LaserValueCounter : public LaserValueFormatBase {
 public:
  LaserValueCounter() : LaserValueFormatBase(ValueType::COUNTER) {}
  LaserValueCounter(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueCounter(int64_t value) : LaserValueFormatBase(ValueType::COUNTER), value_(value) { encode(); }

  inline int64_t getValue() const { return value_; }

  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }

    if (getType() != ValueType::COUNTER) {
      return false;
    }

    if (!unpackInt<int64_t>(&value_)) {
      return false;
    }

    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<int64_t>(value_);
  }

 private:
  int64_t value_{0};
};

class LaserValueZSet : public LaserValueFormatBase {
 public:
  LaserValueZSet() : LaserValueFormatBase(ValueType::ZSET) {}
  LaserValueZSet(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueZSet(const std::vector<std::string>& members)
      : LaserValueFormatBase(ValueType::ZSET), members_(members) {
    encode();
  }
  inline const std::vector<std::string>& getMembers() const { return members_; }
  inline void addMember(const std::string& member) { members_.emplace_back(member); }
  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }
    if (getType() != ValueType::ZSET) {
      return false;
    }

    uint32_t size = 0;
    if (!unpackInt<uint32_t>(&size)) {
      return false;
    }

    for (uint32_t i = 0; i < size; i++) {
      std::string member;
      if (!unpackString(&member)) {
        return false;
      }
      members_.emplace_back(member);
    }

    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<uint32_t>(static_cast<uint32_t>(members_.size()));
    for (auto member : members_) {
      packString(member);
    }
  }

 private:
  std::vector<std::string> members_;
};

class LaserValueMapMeta : public LaserValueFormatBase {
 public:
  LaserValueMapMeta() : LaserValueFormatBase(ValueType::MAP) {}
  LaserValueMapMeta(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueMapMeta(uint32_t size) : LaserValueFormatBase(ValueType::MAP), size_(size) { encode(); }
  inline int64_t getSize() const { return size_; }
  inline void incrSize() { size_++; }
  inline void decrSize() { size_--; }

  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }
    if (getType() != ValueType::MAP) {
      return false;
    }
    if (!unpackInt<uint32_t>(&size_)) {
      return false;
    }
    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<uint32_t>(size_);
  }

 private:
  uint32_t size_{0};
};

class LaserValueListMeta : public LaserValueFormatBase {
 public:
  LaserValueListMeta() : LaserValueFormatBase(ValueType::LIST) {}
  LaserValueListMeta(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueListMeta(int64_t start, int64_t end)
      : LaserValueFormatBase(ValueType::LIST), start_(start), end_(end) {
    encode();
  }
  inline int64_t getStart() { return start_; }
  inline int64_t getEnd() { return end_; }
  inline int64_t getSize() const {
    DCHECK_GE(end_, start_);
    if (end_ == start_) {
      return 0;
    }
    return end_ - start_ - 1;
  }

  inline int64_t popFront() { return ++start_; }
  inline int64_t pushFront() {
    if (start_ == end_) {
      end_++;
    }
    return start_--;
  }
  inline int64_t popBack() { return --end_; }
  inline int64_t pushBack() {
    if (start_ == end_) {
      start_--;
    }
    return end_++;
  }

  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }
    if (getType() != ValueType::LIST) {
      return false;
    }
    if (!unpackInt<int64_t>(&start_)) {
      return false;
    }
    if (!unpackInt<int64_t>(&end_)) {
      return false;
    }
    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<int64_t>(start_);
    packInt<int64_t>(end_);
  }

 private:
  int64_t start_{0};
  int64_t end_{0};
};

class LaserValueSetMeta : public LaserValueFormatBase {
 public:
  LaserValueSetMeta() : LaserValueFormatBase(ValueType::SET) {}
  LaserValueSetMeta(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueSetMeta(uint32_t size) : LaserValueFormatBase(ValueType::SET), size_(size) { encode(); }
  inline int64_t getSize() const { return size_; }
  inline void incrSize() { size_++; }
  inline void decrSize() { size_--; }

  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }
    if (getType() != ValueType::SET) {
      return false;
    }
    if (!unpackInt<uint32_t>(&size_)) {
      return false;
    }
    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<uint32_t>(size_);
  }

 private:
  uint32_t size_{0};
};

class LaserValueZSetMeta : public LaserValueFormatBase {
 public:
  LaserValueZSetMeta() : LaserValueFormatBase(ValueType::ZSET) {}
  LaserValueZSetMeta(const char* buffer, size_t length) : LaserValueFormatBase(buffer, length) {}
  explicit LaserValueZSetMeta(uint32_t size) : LaserValueFormatBase(ValueType::ZSET), size_(size) { encode(); }
  inline uint32_t getSize() const { return size_; }
  inline void incrSize() { size_++; }
  inline void decrSize() { size_--; }

  inline bool decode() override {
    if (!LaserValueFormatBase::decode()) {
      return false;
    }
    if (getType() != ValueType::ZSET) {
      return false;
    }
    if (!unpackInt<uint32_t>(&size_)) {
      return false;
    }

    return true;
  }

  inline void encode() override {
    LaserValueFormatBase::encode();
    packInt<uint32_t>(size_);
  }

 private:
  uint32_t size_{0};
};

}  // namespace laser
