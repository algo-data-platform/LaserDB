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

#include <memory>

#include "common/laser/format.h"
#include "common/util.h"

#include "expire_filter.h"
#include "rocksdb.h"

namespace laser {

constexpr char LASER_ROCKSDB_ENGINE_DELETE_EXPIRE[] = "expire_delete";

ExpireFilter::ExpireFilter() {
  std::unordered_map<std::string, std::string> tags;
  delete_meter_ = metrics::Metrics::getInstance()->buildMeter(LASER_ROCKSDB_ENGINE_MODULE_NAME,
                                                              LASER_ROCKSDB_ENGINE_DELETE_EXPIRE, tags);
}

bool ExpireFilter::Filter(int level, const rocksdb::Slice& key, const rocksdb::Slice& value, std::string* new_value,
                          bool* value_changed) const {
  LaserValueFormatBase val(value.data(), value.size());
  uint64_t current_time = static_cast<uint64_t>(common::currentTimeInMs());
  if (!val.decode()) {
    return false;
  }
  if (val.getTimestamp() > current_time || val.getTimestamp() == 0) {
    return false;
  }
  delete_meter_->mark();
  return true;
}

const char* ExpireFilter::Name() const { return "ExpireFilter"; }

std::unique_ptr<rocksdb::CompactionFilter> ExpireFilterFactory::CreateCompactionFilter(
    const rocksdb::CompactionFilter::Context& context) {
  return std::make_unique<ExpireFilter>();
}

const char* ExpireFilterFactory::Name() const { return "ExpireFilterFactory"; }

}  // namespace laser
