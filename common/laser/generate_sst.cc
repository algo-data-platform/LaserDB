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

#include "generate_sst.h"

namespace laser {

GenerateSst::GenerateSst(const std::string& temp_db_path, const std::string& sst_file_path,
                         const rocksdb::Options& options)
    : sst_file_path_(sst_file_path) {
  auto db = std::make_shared<laser::ReplicationDB>(temp_db_path, options);
  rocksdb::WriteOptions write_options;
  write_options.disableWAL = true;
  db->setWriteOption(write_options);
  db_ = std::make_shared<laser::RocksDbEngine>(db);
}

bool GenerateSst::open() { return db_->open(); }

bool GenerateSst::insertData(const laser::LoaderSourceData& data) {
  switch (data.getValueType()) {
    case laser::LoaderSourceDataType::RAW_STRING:
      return insertRawString(data);
    case laser::LoaderSourceDataType::COUNTER:
      return insertCounter(data);
    case laser::LoaderSourceDataType::MAP:
      return insertMap(data);
    case laser::LoaderSourceDataType::SET:
      return insertSet(data);
    case laser::LoaderSourceDataType::LIST:
      return insertList(data);
    default:
      break;
  }
  return true;
}

bool GenerateSst::dumpSst() {
  laser::Status status = db_->dumpSst(sst_file_path_);
  if (status != laser::Status::OK) {
    LOG(INFO) << "Dump sst file: " << sst_file_path_ << " fail.";
    return false;
  }

  return true;
}

bool GenerateSst::batchInsertRawString(const std::vector<std::shared_ptr<laser::LoaderSourceData>>& data) {
  std::vector<laser::LaserKeyFormat> keys;
  std::vector<std::string> values;
  for (auto& item : data) {
    laser::LaserKeyFormat key(item->getPrimaryKeys(), item->getColumns());
    keys.push_back(key);
    values.push_back(item->getRawStringData());
  }
  laser::Status status = db_->mset(keys, values);
  if (status != laser::Status::OK) {
    LOG(INFO) << "Batch insert raw string data fail";
    return false;
  }
  return true;
}

bool GenerateSst::insertRawString(const laser::LoaderSourceData& data) {
  laser::LaserKeyFormat key(data.getPrimaryKeys(), data.getColumns());
  laser::Status status = db_->set(key, data.getRawStringData());
  if (status != laser::Status::OK) {
    LOG(INFO) << "Insert raw string data fail, data:" << data;
    return false;
  }
  return true;
}

bool GenerateSst::insertCounter(const laser::LoaderSourceData& data) {
  laser::LaserKeyFormat key(data.getPrimaryKeys(), data.getColumns());
  int64_t value;
  laser::Status status = db_->incr(&value, key, data.getCounterData());
  if (status != laser::Status::OK) {
    LOG(INFO) << "Insert counter data fail, data:" << data;
    return false;
  }
  return true;
}

bool GenerateSst::insertMap(const laser::LoaderSourceData& data) {
  laser::LaserKeyFormat key(data.getPrimaryKeys(), data.getColumns());
  laser::Status status;
  for (auto& t : data.getMapData()) {
    status = db_->hset(key, t.first, t.second);
    if (status != laser::Status::OK) {
      LOG(INFO) << "Insert map data fail, data:" << data << " key:" << t.first << " value:" << t.second;
      return false;
    }
  }

  return true;
}

bool GenerateSst::insertSet(const laser::LoaderSourceData& data) {
  laser::LaserKeyFormat key(data.getPrimaryKeys(), data.getColumns());
  laser::Status status;
  for (auto& t : data.getSetData()) {
    status = db_->sadd(key, t);
    if (status != laser::Status::OK) {
      LOG(INFO) << "Insert set data fail, data:" << data << " member:" << t;
      return false;
    }
  }
  return true;
}

bool GenerateSst::insertList(const laser::LoaderSourceData& data) {
  laser::LaserKeyFormat key(data.getPrimaryKeys(), data.getColumns());
  laser::Status status;
  for (auto& t : data.getListData()) {
    status = db_->pushBack(key, t);
    if (status != laser::Status::OK) {
      LOG(INFO) << "Insert list data fail, data:" << data << " item:" << t;
      return false;
    }
  }
  return true;
}

}  // namespace laser
