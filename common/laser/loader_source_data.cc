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

#include "loader_source_data.h"

namespace laser {

std::ostream& operator<<(std::ostream& os, const LoaderSourceDataType& value) {
  switch (value) {
    case LoaderSourceDataType::RAW_STRING:
      os << "raw_string";
      break;
    case LoaderSourceDataType::COUNTER:
      os << "counter";
      break;
    case LoaderSourceDataType::MAP:
      os << "map";
      break;
    case LoaderSourceDataType::SET:
      os << "set";
      break;
    case LoaderSourceDataType::LIST:
      os << "list";
      break;
    default:
      os << "type unknow";
  }
  return os;
}

folly::dynamic serializeLoaderSourceDataType(const LoaderSourceDataType& value) {
  folly::dynamic result = toStringLoaderSourceDataType(value);
  return result;
}

bool deserializeLoaderSourceDataType(const folly::dynamic& data, LoaderSourceDataType* result) {
  if (!data.isString()) {
    return false;
  }

  std::string value = data.asString();
  auto enum_obj = stringToLoaderSourceDataType(value);
  if (enum_obj) {
    *result = *enum_obj;
    return true;
  }

  return false;
}

folly::Optional<LoaderSourceDataType> stringToLoaderSourceDataType(const std::string& name) {
  if (name == "raw_string") {
    return LoaderSourceDataType::RAW_STRING;
  }
  if (name == "counter") {
    return LoaderSourceDataType::COUNTER;
  }
  if (name == "map") {
    return LoaderSourceDataType::MAP;
  }
  if (name == "set") {
    return LoaderSourceDataType::SET;
  }
  if (name == "list") {
    return LoaderSourceDataType::LIST;
  }

  return folly::none;
}

const std::string toStringLoaderSourceDataType(const LoaderSourceDataType& value) {
  std::string result;
  switch (value) {
    case LoaderSourceDataType::RAW_STRING:
      result = "raw_string";
      break;
    case LoaderSourceDataType::COUNTER:
      result = "counter";
      break;
    case LoaderSourceDataType::MAP:
      result = "map";
      break;
    case LoaderSourceDataType::SET:
      result = "set";
      break;
    case LoaderSourceDataType::LIST:
      result = "list";
      break;
    default:
      result = "unknow";
  }

  return result;
}

void LoaderSourceData::describe(std::ostream& os) const {
  os << "LoaderSourceData{"
     << "PrimaryKeys=[";
  for (auto& t : primary_keys_) {
    os << t << ",";
  }
  os << "]"
     << ", "
     << "Columns=[";
  for (auto& t : columns_) {
    os << t << ",";
  }
  os << "]"
     << ", "
     << "ValueType=" << value_type_ << ", "
     << "RawStringData='" << raw_string_data_ << "'"
     << ", "
     << "CounterData=" << counter_data_ << ", "
     << "MapData={";
  for (auto& t : map_data_) {
    os << t.first << "=" << t.second << ",";
  }
  os << "}"
     << ", "
     << "SetData=[";
  for (auto& t : set_data_) {
    os << t << ",";
  }
  os << "]"
     << ", "
     << "ListData=[";
  for (auto& t : list_data_) {
    os << t << ",";
  }
  os << "]"
     << "}";
}

std::ostream& operator<<(std::ostream& os, const LoaderSourceData& value) {
  value.describe(os);
  return os;
}

const folly::dynamic LoaderSourceData::serialize() const {
  folly::dynamic result = folly::dynamic::object;

  folly::dynamic primary_keys = folly::dynamic::array;
  for (auto& t : primary_keys_) {
    primary_keys.push_back(t);
  }
  result.insert("PrimaryKeys", primary_keys);
  folly::dynamic columns = folly::dynamic::array;
  for (auto& t : columns_) {
    columns.push_back(t);
  }
  result.insert("Columns", columns);
  folly::dynamic value_type = serializeLoaderSourceDataType(value_type_);
  result.insert("ValueType", value_type);
  switch (value_type_) {
    case LoaderSourceDataType::RAW_STRING:
      result.insert("Data", raw_string_data_);
      break;
    case LoaderSourceDataType::COUNTER:
      result.insert("Data", counter_data_);
      break;
    case LoaderSourceDataType::MAP: {
      folly::dynamic map_data = folly::dynamic::object;
      for (auto& t : map_data_) {
        map_data.insert(t.first, t.second);
      }
      result.insert("Data", map_data);
    } break;
    case LoaderSourceDataType::SET: {
      folly::dynamic set_data = folly::dynamic::array;
      for (auto& t : set_data_) {
        set_data.push_back(t);
      }
      result.insert("Data", set_data);
    } break;
    case LoaderSourceDataType::LIST: {
      folly::dynamic list_data = folly::dynamic::array;
      for (auto& t : list_data_) {
        list_data.push_back(t);
      }
      result.insert("Data", list_data);
    } break;
    default:
      break;
  }

  return result;
}

bool LoaderSourceData::deserialize(const folly::dynamic& data) {
  if (!data.isObject()) {
    return false;
  }

  auto* primary_keys = data.get_ptr("PrimaryKeys");
  if (primary_keys == nullptr || !primary_keys->isArray()) {
    return false;
  }

  std::vector<std::string> vec_primary_keys;
  for (size_t i = 0; i < primary_keys->size(); i++) {
    if (!primary_keys->at(i).isString()) {
      return false;
    }
    vec_primary_keys.push_back(primary_keys->at(i).asString());
  }
  setPrimaryKeys(vec_primary_keys);
  auto* columns = data.get_ptr("Columns");
  if (columns == nullptr || !columns->isArray()) {
    return false;
  }

  std::vector<std::string> vec_columns;
  for (size_t i = 0; i < columns->size(); i++) {
    if (!columns->at(i).isString()) {
      return false;
    }
    vec_columns.push_back(columns->at(i).asString());
  }
  setColumns(vec_columns);
  auto* value_type = data.get_ptr("ValueType");
  if (value_type == nullptr) {
    return false;
  }

  LoaderSourceDataType itemvalue_type;
  if (!deserializeLoaderSourceDataType(*value_type, &itemvalue_type)) {
    return false;
  }
  setValueType(itemvalue_type);

  switch (value_type_) {
    case LoaderSourceDataType::RAW_STRING: {
      auto* raw_string_data = data.get_ptr("Data");
      if (raw_string_data == nullptr || !raw_string_data->isString()) {
        return false;
      }
      setRawStringData(raw_string_data->asString());
    } break;
    case LoaderSourceDataType::COUNTER: {
      auto* counter_data = data.get_ptr("Data");
      if (counter_data == nullptr || !counter_data->isInt()) {
        return false;
      }
      setCounterData(counter_data->asInt());
    } break;
    case LoaderSourceDataType::MAP: {
      auto* map_data = data.get_ptr("Data");
      if (map_data == nullptr || !map_data->isObject()) {
        return false;
      }

      std::unordered_map<std::string, std::string> map_map_data;
      for (auto iter = map_data->keys().begin(); iter != map_data->keys().end(); iter++) {
        if (!map_data->at(*iter).isString()) {
          return false;
        }

        if (!iter->isString()) {
          return false;
        }
        map_map_data.insert({iter->asString(), map_data->at(*iter).asString()});
      }
      setMapData(map_map_data);
    } break;
    case LoaderSourceDataType::SET: {
      auto* set_data = data.get_ptr("Data");
      if (set_data == nullptr || !set_data->isArray()) {
        return false;
      }
      std::vector<std::string> vec_set_data;
      for (size_t i = 0; i < set_data->size(); i++) {
        if (!set_data->at(i).isString()) {
          return false;
        }
        vec_set_data.push_back(set_data->at(i).asString());
      }
      setSetData(vec_set_data);
    } break;
    case LoaderSourceDataType::LIST: {
      auto* list_data = data.get_ptr("Data");
      if (list_data == nullptr || !list_data->isArray()) {
        return false;
      }

      std::vector<std::string> vec_list_data;
      for (size_t i = 0; i < list_data->size(); i++) {
        if (!list_data->at(i).isString()) {
          return false;
        }
        vec_list_data.push_back(list_data->at(i).asString());
      }
      setListData(vec_list_data);
    } break;
    default:
      VLOG(5) << "data type invalid";
  }
  return true;
}

}  // namespace laser
