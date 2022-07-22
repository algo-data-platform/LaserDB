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

#include "rocksdb/compaction_filter.h"
#include "rocksdb/slice.h"
#include "common/metrics/metrics.h"

namespace laser {
class ExpireFilter : public rocksdb::CompactionFilter {
 public:
  ExpireFilter();
  ~ExpireFilter() = default;
  bool Filter(int level, const rocksdb::Slice& key, const rocksdb::Slice& value, std::string* new_value,
              bool* value_changed) const;
  const char* Name() const;

 private:
  std::shared_ptr<metrics::Meter> delete_meter_;
};

class ExpireFilterFactory : public rocksdb::CompactionFilterFactory {
 public:
  ExpireFilterFactory() = default;
  std::unique_ptr<rocksdb::CompactionFilter> CreateCompactionFilter(const rocksdb::CompactionFilter::Context& context);
  const char* Name() const;
};

}  // namespace laser
