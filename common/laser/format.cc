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
 */

#include "format.h"

#include "common/util.h"

namespace laser {

static const ValueType intToValueType(uint8_t type) {
  switch (type) {
    case 2:
      return ValueType::COUNTER;
    case 3:
      return ValueType::MAP;
    case 4:
      return ValueType::LIST;
    case 5:
      return ValueType::SET;
    case 6:
      return ValueType::ZSET;
    default:
      return ValueType::RAW_STRING;
  }
}

static const KeyType intToKeyType(uint8_t type) {
  switch (type) {
    case 2:
      return KeyType::COMPOSITE;
    default:
      return KeyType::DEFAULT;
  }
}

const std::string LaserSerializer::toHexString(std::string tok) {
  std::string output;
  char temp[8];
  for (size_t i = 0; i < raw_data_.length(); i++) {
    snprintf(temp, sizeof(temp), "0x%.2x", raw_data_.at(i));
    output.append(temp, 4);
    output.append(tok);
  }
  return output;
}

void LaserSerializer::packString(const std::string& str) {
  if (str.empty()) {
    return;
  }
  packInt<uint32_t>(static_cast<uint32_t>(str.size()));
  raw_data_.append(str);
}

void LaserSerializer::packString(const char* data, size_t size) {
  if (size == 0) {
    return;
  }
  packInt<uint32_t>(static_cast<uint32_t>(size));
  raw_data_.append(data, size);
}

bool LaserSerializer::unpackString(std::string* result) {
  uint32_t size = 0;
  if (!unpackInt<uint32_t>(&size)) {
    return false;
  }
  if (raw_data_.length() < (offset_ + size)) {
    return false;
  }
  result->append(raw_data_.data() + offset_, static_cast<size_t>(size));
  offset_ += size;

  return true;
}

void LaserKeyFormatBase::encode() {
  clear();
  packInt<uint8_t>(static_cast<uint8_t>(key_type_));
  packInt<uint32_t>(static_cast<uint32_t>(primary_keys_.size()));
  for (auto& t : primary_keys_) {
    packString(t);
  }
  packInt<uint32_t>(static_cast<uint32_t>(columns_.size()));
  for (auto& t : columns_) {
    packString(t);
  }
}

bool LaserKeyFormatBase::decode() {
  resetOffset();
  uint8_t type_int = 0;
  if (!unpackInt<uint8_t>(&type_int)) {
    return false;
  }
  key_type_ = intToKeyType(type_int);
  uint32_t size = 0;
  if (!unpackInt<uint32_t>(&size)) {
    return false;
  }

  for (int i = 0; i < size; i++) {
    std::string unpack_str;
    if (!unpackString(&unpack_str)) {
      return false;
    }
    primary_keys_.push_back(unpack_str);
  }
  if (!unpackInt<uint32_t>(&size)) {
    return false;
  }
  for (int i = 0; i < size; i++) {
    std::string unpack_str;
    if (!unpackString(&unpack_str)) {
      return false;
    }
    columns_.push_back(unpack_str);
  }

  return true;
}

void LaserKeyFormatTtl::encode() {
  clear();
  packInt<uint8_t>(static_cast<uint8_t>(key_type_));
  packString(folly::to<std::string>(timestamp_));
  packString(key_.data(), key_.length());
}

bool LaserKeyFormatTtl::decode() {
  resetOffset();
  uint8_t type_int = 0;
  if (!unpackInt<uint8_t>(&type_int)) {
    return false;
  }

  std::string timestamp_str;
  if (!unpackString(&timestamp_str)) {
    return false;
  }

  if (!timestamp_str.empty()) {
    auto timestamp_opt = folly::tryTo<uint64_t>(timestamp_str);
    if (timestamp_opt.hasValue()) {
      timestamp_ = timestamp_opt.value();
    }
  }

  std::string key;
  if (!unpackString(&key)) {
    return false;
  }

  LaserKeyFormat key_format(key.data(), key.size());
  if (!key_format.decode()) {
    return false;
  }
  key_ = key_format;

  return true;
}

LaserValueFormatBase::LaserValueFormatBase(const ValueType& type, uint64_t timestamp)
    : type_(type), timestamp_(timestamp) {}

void LaserValueFormatBase::encode() {
  clear();
  packInt<uint8_t>(static_cast<uint8_t>(type_));
  packInt<uint64_t>(timestamp_);
}

bool LaserValueFormatBase::decode() {
  resetOffset();
  uint8_t int_type = 0;
  if (!unpackInt<uint8_t>(&int_type)) {
    return false;
  }
  type_ = intToValueType(int_type);
  if (!unpackInt<uint64_t>(&timestamp_)) {
    return false;
  }
  return true;
}

void LaserValueRawString::encode() {
  LaserValueFormatBase::encode();
  packString(value_);
}

bool LaserValueRawString::decode() {
  if (!LaserValueFormatBase::decode()) {
    return false;
  }

  value_.clear();

  if (getType() != ValueType::RAW_STRING) {
    return false;
  }

  if (!unpackString(&value_)) {
    return false;
  }
  return true;
}

}  // namespace laser
