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

#pragma once

#include "laser_entity.h"

#include "folly/Synchronized.h"

namespace laser {

enum class LoaderSourceDataType {
  RAW_STRING,
  COUNTER,
  MAP,
  SET,
  LIST
};

folly::dynamic serializeLoaderSourceDataType(const LoaderSourceDataType& value);

bool deserializeLoaderSourceDataType(const folly::dynamic& data, LoaderSourceDataType* value);

std::ostream& operator<<(std::ostream& os, const LoaderSourceDataType& value);

folly::Optional<LoaderSourceDataType> stringToLoaderSourceDataType(const std::string& name);

const std::string toStringLoaderSourceDataType(const LoaderSourceDataType& value);

class LoaderSourceData {
 public:
  LoaderSourceData() = default;
  ~LoaderSourceData() = default;

  const std::vector<std::string>& getPrimaryKeys() const { return primary_keys_; }

  void setPrimaryKeys(const std::vector<std::string>& primary_keys) { primary_keys_ = primary_keys; }

  const std::vector<std::string>& getColumns() const { return columns_; }

  void setColumns(const std::vector<std::string>& columns) { columns_ = columns; }

  const LoaderSourceDataType& getValueType() const { return value_type_; }

  void setValueType(const LoaderSourceDataType& value_type) { value_type_ = value_type; }

  const std::string& getRawStringData() const { return raw_string_data_; }

  void setRawStringData(const std::string& raw_string_data) { raw_string_data_ = raw_string_data; }

  int64_t getCounterData() const { return counter_data_; }

  void setCounterData(int64_t counter_data) { counter_data_ = counter_data; }

  const std::unordered_map<std::string, std::string>& getMapData() const { return map_data_; }

  void setMapData(const std::unordered_map<std::string, std::string>& map_data) { map_data_ = map_data; }

  const std::vector<std::string>& getSetData() const { return set_data_; }

  void setSetData(const std::vector<std::string>& set_data) { set_data_ = set_data; }

  const std::vector<std::string>& getListData() const { return list_data_; }

  void setListData(const std::vector<std::string>& list_data) { list_data_ = list_data; }

  void describe(std::ostream& os) const;

  const folly::dynamic serialize() const;

  bool deserialize(const folly::dynamic& data);

 private:
  std::vector<std::string> primary_keys_;
  std::vector<std::string> columns_;
  LoaderSourceDataType value_type_;
  std::string raw_string_data_;
  int64_t counter_data_;
  std::unordered_map<std::string, std::string> map_data_;
  std::vector<std::string> set_data_;
  std::vector<std::string> list_data_;
};

std::ostream& operator<<(std::ostream& os, const LoaderSourceData& value);

}  // namespace laser
