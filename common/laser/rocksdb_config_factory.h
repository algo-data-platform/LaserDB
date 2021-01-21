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
 */

#pragma once

#include "folly/SpinLock.h"
#include "rocksdb/write_buffer_manager.h"
#include "rocksdb/options.h"
#include "rocksdb/convenience.h"
#include "rocksdb/table.h"
#include "rocksdb/statistics.h"
#include "folly/Optional.h"

#include "config_manager.h"
#include "versioned_options.h"

namespace laser {

class RocksDbStatistics {
 public:
  RocksDbStatistics() { statistics_ = rocksdb::CreateDBStatistics(); }
  ~RocksDbStatistics() = default;
  void init();
  inline std::shared_ptr<rocksdb::Statistics> getStatistics() { return statistics_; }

 private:
  std::shared_ptr<rocksdb::Statistics> statistics_;
};

using RocksDbConfigUpdateCallback = folly::Function<void()>;
class RocksDbConfigFactory : public std::enable_shared_from_this<RocksDbConfigFactory> {
 public:
  explicit RocksDbConfigFactory(std::shared_ptr<ConfigManager> config_manager) : config_manager_(config_manager) {}
  virtual ~RocksDbConfigFactory() {}
  virtual void init(RocksDbConfigUpdateCallback callback, folly::EventBase* evb);
  virtual bool update(const std::shared_ptr<NodeConfig> node_config, const TableConfigList config_list,
                      const TableSchemasMap table_schemas);
  virtual const folly::Optional<rocksdb::Options> getDefaultOptions() const;
  virtual const folly::Optional<rocksdb::Options> getOptions(const std::string& db_name,
                                                             const std::string& table_name) const;
  virtual const folly::Optional<VersionedOptions> getVersionedOptions(const std::string& db_name,
                                                                      const std::string& table_name) const;

  virtual bool hasOptionsChanged(const std::string& db_name, const std::string& table_name,
                                 const VersionedOptions& versioned_options) const;

 private:
  std::shared_ptr<ConfigManager> config_manager_;
  std::shared_ptr<rocksdb::Cache> cache_;
  std::shared_ptr<RocksDbStatistics> statistics_;
  std::shared_ptr<rocksdb::WriteBufferManager> write_buffer_manager_;
  std::shared_ptr<VersionedOptions> default_options_;
  std::unordered_map<uint64_t, std::shared_ptr<VersionedOptions>> table_options_;
  std::shared_ptr<rocksdb::RateLimiter> rate_limiter_;
  folly::Synchronized<std::map<uint32_t, int64_t>> begin_hour_to_rate_bytes_;
  folly::EventBase* timer_evb_;
  std::atomic<bool> has_inited_{false};
  mutable folly::SpinLock spinlock_;
  RocksDbConfigUpdateCallback update_callback_;

  bool initGlobalConfig(std::shared_ptr<NodeConfig> config);
  bool initOption(const TableConfigList& config_list, const TableSchemasMap& table_schemas);
  bool getOptionFromTableConfig(const TableConfig& config, const std::shared_ptr<rocksdb::Options>& option);
  void initRateLimit(const std::shared_ptr<NodeConfig> config);
  void updateRateLimit();
  uint64 getTableHash(const std::string& db_name, const std::string& table_name) const;
};

}  // namespace laser
