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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#include "rocksdb_config_factory.h"
#include "common/metrics/metrics.h"
#include "rocksdb/sst_file_manager.h"
#include "rocksdb/rate_limiter.h"

namespace laser {

constexpr static char LASER_METRICS_MODULE_NAME_FOR_ROCKSDB[] = "rocksdb";
constexpr static char METRIC_ROCKSDB_STAT_TYPE[] = "rocksdb_stat_type";
constexpr static char METRIC_ROCKSDB_STAT_TICKER[] = "ticker";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM[] = "histogram";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM[] = "item";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_MEDIAN[] = "median";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_STANDARD_DEVIATION[] = "standard_deviation";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_PERCENTILE99[] = "percentile99";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_PERCENTILE95[] = "percentile95";
constexpr static char METRIC_ROCKSDB_STAT_HISTOGRAM_AVERAGE[] = "average";
constexpr static uint32_t METRIC_ROCKSDB_STAT_INTERVAL = 10000;
constexpr static char LASER_DEFAULT_TABLE_OPTION_KEY[] = "default";
constexpr static char ROCKSDB_DBOPTIONS_USE_DIRECT_READS[] = "use_direct_reads";
constexpr static uint32_t HOURS_IN_A_DAY = 24;

DEFINE_bool(force_disable_direct_reads, false, "Disable direct read on machine which do not support this feature");
DEFINE_int32(max_background_flushes, 8, "Max background flushes of rocksdb");
DEFINE_int32(max_background_compactions, 8, "Max background compactions of rocksdb");
DEFINE_int64(rocksdb_rate_bytes_per_sec, 800 * 1024 * 1024, "Rate bytes per second for rocksdb rate limiter");
DEFINE_uint64(periodic_compaction_seconds, 2592000, "total compaction process every 30 days");

void RocksDbStatistics::init() {
  auto metrics = metrics::Metrics::getInstance();
  std::weak_ptr<rocksdb::Statistics> stat = statistics_;

  // ticker 类型
  for (auto& ticker : rocksdb::TickersNameMap) {
    std::unordered_map<std::string, std::string> tags = {{METRIC_ROCKSDB_STAT_TYPE, METRIC_ROCKSDB_STAT_TICKER}};
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, ticker.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, ticker]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           return static_cast<double>(stat_ptr->getTickerCount(ticker.first));
                         },
                         tags);
  }

  // histogram
  for (auto& histogram : rocksdb::HistogramsNameMap) {
    std::unordered_map<std::string, std::string> tags = {{METRIC_ROCKSDB_STAT_TYPE, METRIC_ROCKSDB_STAT_HISTOGRAM}};
    tags[METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM] = METRIC_ROCKSDB_STAT_HISTOGRAM_MEDIAN;
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, histogram.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, histogram]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           rocksdb::HistogramData data;
                           stat_ptr->histogramData(histogram.first, &data);
                           return data.median;
                         },
                         tags);
    tags[METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM] = METRIC_ROCKSDB_STAT_HISTOGRAM_PERCENTILE95;
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, histogram.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, histogram]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           rocksdb::HistogramData data;
                           stat_ptr->histogramData(histogram.first, &data);
                           return data.percentile95;
                         },
                         tags);
    tags[METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM] = METRIC_ROCKSDB_STAT_HISTOGRAM_PERCENTILE99;
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, histogram.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, histogram]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           rocksdb::HistogramData data;
                           stat_ptr->histogramData(histogram.first, &data);
                           return data.percentile99;
                         },
                         tags);
    tags[METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM] = METRIC_ROCKSDB_STAT_HISTOGRAM_AVERAGE;
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, histogram.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, histogram]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           rocksdb::HistogramData data;
                           stat_ptr->histogramData(histogram.first, &data);
                           return data.average;
                         },
                         tags);
    tags[METRIC_ROCKSDB_STAT_HISTOGRAM_ITEM] = METRIC_ROCKSDB_STAT_HISTOGRAM_STANDARD_DEVIATION;
    metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, histogram.second, METRIC_ROCKSDB_STAT_INTERVAL,
                         [stat, histogram]() {
                           auto stat_ptr = stat.lock();
                           if (!stat_ptr) {
                             return 0.0;
                           }
                           rocksdb::HistogramData data;
                           stat_ptr->histogramData(histogram.first, &data);
                           return data.standard_deviation;
                         },
                         tags);
  }
}

void RocksDbConfigFactory::init(RocksDbConfigUpdateCallback callback, folly::EventBase* evb) {
  update_callback_ = std::move(callback);
  timer_evb_ = evb;
  config_manager_->subscribeRocksDbConfig([this](auto node_config, auto table_config_list, auto table_schemas) {
    if (update(node_config, table_config_list, table_schemas)) {
      update_callback_();
    }
  });
}

bool RocksDbConfigFactory::update(const std::shared_ptr<NodeConfig> node_config, const TableConfigList config_list,
                                  const TableSchemasMap table_schemas) {
  folly::SpinLockGuard g(spinlock_);
  // 全局配置仅仅在启动时第一次创建，后续配置更改不受影响
  if (!has_inited_) {
    if (!initGlobalConfig(node_config)) {
      return false;
    }
    has_inited_ = true;
  }
  initRateLimit(node_config);

  return initOption(config_list, table_schemas);
}

const folly::Optional<rocksdb::Options> RocksDbConfigFactory::getDefaultOptions() const {
  folly::SpinLockGuard g(spinlock_);
  if (!default_options_) {
    return folly::none;
  }
  return default_options_->getOptions();
}

const folly::Optional<rocksdb::Options> RocksDbConfigFactory::getOptions(const std::string& db_name,
                                                                         const std::string& table_name) const {
  auto versioned_option = getVersionedOptions(db_name, table_name);
  if (!versioned_option.hasValue()) {
    return folly::none;
  }

  return versioned_option.value().getOptions();
}

const folly::Optional<VersionedOptions> RocksDbConfigFactory::getVersionedOptions(const std::string& db_name,
                                                                                  const std::string& table_name) const {
  folly::SpinLockGuard g(spinlock_);
  auto key = getTableHash(db_name, table_name);
  auto found = table_options_.find(key);
  if (found != table_options_.end()) {
    return *found->second;
  }

  if (!default_options_) {
    return folly::none;
  }
  return *default_options_;
}

bool RocksDbConfigFactory::hasOptionsChanged(const std::string& db_name, const std::string& table_name,
                                             const VersionedOptions& versioned_options) const {
  auto key = getTableHash(db_name, table_name);
  folly::SpinLockGuard g(spinlock_);
  auto found = table_options_.find(key);
  if (found != table_options_.end()) {
    if (found->second->getVersionHash() != versioned_options.getVersionHash()) {
      return true;
    }
    return false;
  }

  if (default_options_->getVersionHash() == versioned_options.getVersionHash()) {
    return false;
  }
  return true;
}

bool RocksDbConfigFactory::initGlobalConfig(std::shared_ptr<NodeConfig> config) {
  if (!config) {
    LOG(ERROR) << "Rocksdb node config is empty!!!";
    return false;
  }
  LOG(INFO) << "Rocksdb node config:" << *config;
  uint64_t cache_size = static_cast<uint64_t>(config->getBlockCacheSizeGb()) * 1024 * 1024 * 1024;
  uint64_t write_buffer_size = static_cast<uint64_t>(config->getWriteBufferSizeGb()) * 1024 * 1024 * 1024;
  int32_t num_shard_bits = config->getNumShardBits();
  bool strict_capacity_limit = config->getStrictCapacityLimit();
  double high_pri_pool_ratio = config->getHighPriPoolRatio();
  cache_ = rocksdb::NewLRUCache(cache_size, num_shard_bits, strict_capacity_limit, high_pri_pool_ratio);
  if (cache_ == nullptr) {
    LOG(ERROR) << "NewLRUCache failed!!! Rocksdb node config:" << *config;
    return false;
  }
  LOG(INFO) << "Rocksdb block cache capacity size:" << cache_->GetCapacity() << " num shard bits:" << num_shard_bits
            << " strict_capacity_limit:" << strict_capacity_limit << " high_pri_pool_ratio:" << high_pri_pool_ratio;

  // write buffer 使用 block cache 的物理内存
  write_buffer_manager_ = std::make_shared<rocksdb::WriteBufferManager>(write_buffer_size, cache_);
  statistics_ = std::make_shared<RocksDbStatistics>();
  statistics_->init();

  rate_limiter_ =
      std::shared_ptr<rocksdb::RateLimiter>(rocksdb::NewGenericRateLimiter(FLAGS_rocksdb_rate_bytes_per_sec));
  LOG(INFO) << "Set default rate bytes per second of rate limiter to " << FLAGS_rocksdb_rate_bytes_per_sec;

  auto metrics = metrics::Metrics::getInstance();
  std::weak_ptr<rocksdb::Cache> weak_cache = cache_;
  std::weak_ptr<rocksdb::WriteBufferManager> weak_buffer = write_buffer_manager_;
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "cache.usage", METRIC_ROCKSDB_STAT_INTERVAL,
                       [weak_cache]() {
                         auto cache = weak_cache.lock();
                         if (!cache) {
                           return 0.0;
                         }
                         return static_cast<double>(cache->GetUsage());
                       });
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "cache.capacity", METRIC_ROCKSDB_STAT_INTERVAL,
                       [weak_cache]() {
                         auto cache = weak_cache.lock();
                         if (!cache) {
                           return 0.0;
                         }
                         return static_cast<double>(cache->GetCapacity());
                       });
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "cache.pin_usage", METRIC_ROCKSDB_STAT_INTERVAL,
                       [weak_cache]() {
                         auto cache = weak_cache.lock();
                         if (!cache) {
                           return 0.0;
                         }
                         return static_cast<double>(cache->GetPinnedUsage());
                       });
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "buffer.memory_usage", METRIC_ROCKSDB_STAT_INTERVAL,
                       [weak_buffer]() {
                         auto buffer = weak_buffer.lock();
                         if (!buffer) {
                           return 0.0;
                         }
                         return static_cast<double>(buffer->memory_usage());
                       });
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "buffer.mutable_memtable_memory_usage",
                       METRIC_ROCKSDB_STAT_INTERVAL, [weak_buffer]() {
                         auto buffer = weak_buffer.lock();
                         if (!buffer) {
                           return 0.0;
                         }
                         return static_cast<double>(buffer->mutable_memtable_memory_usage());
                       });
  metrics->buildGauges(LASER_METRICS_MODULE_NAME_FOR_ROCKSDB, "buffer.buffer_size", METRIC_ROCKSDB_STAT_INTERVAL,
                       [weak_buffer]() {
                         auto buffer = weak_buffer.lock();
                         if (!buffer) {
                           return 0.0;
                         }
                         return static_cast<double>(buffer->buffer_size());
                       });
  return true;
}

bool RocksDbConfigFactory::initOption(const TableConfigList& config_list, const TableSchemasMap& table_schemas) {
  std::unordered_map<uint64_t, std::shared_ptr<VersionedOptions>> new_table_options;
  for (auto& schema : table_schemas) {
    auto config_iter = config_list.getTableConfigList().find(schema.second->getConfigName());
    if (config_iter == config_list.getTableConfigList().end()) {
      continue;
    }

    auto key = getTableHash(schema.second->getDatabaseName(), schema.second->getTableName());
    auto option = std::make_shared<rocksdb::Options>();
    if (!getOptionFromTableConfig(config_iter->second, option)) {
      auto found = table_options_.find(key);
      if (found != table_options_.end()) {
        new_table_options[key] = found->second;
      }
      continue;
    }

    auto versioned_options =
        std::make_shared<VersionedOptions>(option, config_iter->first, config_iter->second.getVersion());
    new_table_options[key] = versioned_options;
  }
  table_options_ = new_table_options;

  auto default_config_iter = config_list.getTableConfigList().find(LASER_DEFAULT_TABLE_OPTION_KEY);
  if (default_config_iter == config_list.getTableConfigList().end()) {
    LOG(ERROR) << "Can not found default table config, value: " << config_list
               << ", default key: " << LASER_DEFAULT_TABLE_OPTION_KEY;
    return false;
  }
  auto option = std::make_shared<rocksdb::Options>();
  if (!getOptionFromTableConfig(default_config_iter->second, option)) {
    LOG(ERROR) << "Parse default options failed! value: " << default_config_iter->second;
    return false;
  }
  default_options_ =
      std::make_shared<VersionedOptions>(option, default_config_iter->first, default_config_iter->second.getVersion());
  return true;
}

bool RocksDbConfigFactory::getOptionFromTableConfig(const TableConfig& config,
                                                    const std::shared_ptr<rocksdb::Options>& option) {
  auto option_configs = config.getDbOptions();
  if (FLAGS_force_disable_direct_reads) {
    option_configs[ROCKSDB_DBOPTIONS_USE_DIRECT_READS] = "false";
    LOG(INFO) << "Force disable dboption of use_direct_reads!";
  }
  rocksdb::Status status = rocksdb::GetDBOptionsFromMap(*option, option_configs, option.get());
  if (!status.ok()) {
    LOG(ERROR) << "Init rocksdb db options fail, error:" << status.ToString();
    return false;
  }
  status = rocksdb::GetColumnFamilyOptionsFromMap(*option, config.getCfOptions(), option.get());
  if (!status.ok()) {
    LOG(ERROR) << "Init rocksdb columns family options fail, error:" << status.ToString();
    return false;
  }

  rocksdb::BlockBasedTableOptions table_options;
  status = rocksdb::GetBlockBasedTableOptionsFromMap(table_options, config.getTableOptions(), &table_options);
  if (!status.ok()) {
    LOG(ERROR) << "Init rocksdb table options fail, error:" << status.ToString();
    return false;
  }

  table_options.block_cache = cache_;
  option->table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
  option->write_buffer_manager = write_buffer_manager_;
  option->statistics = statistics_->getStatistics();
  option->sst_file_manager =
      std::shared_ptr<rocksdb::SstFileManager>(rocksdb::NewSstFileManager(option->env, option->info_log));
  option->max_background_flushes = FLAGS_max_background_flushes;
  option->max_background_compactions = FLAGS_max_background_compactions;
  option->rate_limiter = rate_limiter_;
  option->periodic_compaction_seconds = FLAGS_periodic_compaction_seconds;

  return true;
}

void RocksDbConfigFactory::initRateLimit(const std::shared_ptr<NodeConfig> config) {
  if (!config) {
    LOG(ERROR) << "Rocksdb node config is empty!!!";
    return;
  }
  LOG(INFO) << "Rocksdb node config:" << *config;
  auto rate_limit_strategy = config->getRateLimitStrategy();
  begin_hour_to_rate_bytes_.withWLock([&rate_limit_strategy](auto& wlock) {
    wlock.clear();
    for (auto& entry : rate_limit_strategy) {
      wlock[entry.getBeginHour()] = entry.getRateBytesPerSec();
      if (wlock.find(entry.getEndHour()) == wlock.end()) {
        wlock[entry.getEndHour()] = FLAGS_rocksdb_rate_bytes_per_sec;
      }
    }
  });
  updateRateLimit();
}

void RocksDbConfigFactory::updateRateLimit() {
  LOG(INFO) << "Update rate limit";
  auto now = std::chrono::system_clock::now();
  std::time_t tt_now = std::chrono::system_clock::to_time_t(now);
  tm* local_tm = std::localtime(&tt_now);
  int this_hour = local_tm->tm_hour;

  int64_t rate_bytes_per_sec = FLAGS_rocksdb_rate_bytes_per_sec;
  uint32_t next_update_hours = 0;
  begin_hour_to_rate_bytes_.withRLock([this_hour, &rate_bytes_per_sec, &next_update_hours](auto& rlock) {
    if (rlock.empty()) {
      return;
    }
    auto bound = rlock.lower_bound(this_hour);
    do {
      if (bound != rlock.end() && bound->first == static_cast<uint32_t>(this_hour)) {
        rate_bytes_per_sec = bound->second;
        break;
      }

      if (bound == rlock.begin()) {
        break;
      }

      bound = std::prev(bound);
      rate_bytes_per_sec = bound->second;
    } while (false);
    auto next_bound = std::next(bound);
    if (next_bound == rlock.end()) {
      next_update_hours = HOURS_IN_A_DAY - this_hour + rlock.begin()->first;
    } else {
      next_update_hours = next_bound->first - this_hour;
    }
  });

  if (rate_limiter_->GetBytesPerSecond() != rate_bytes_per_sec) {
    rate_limiter_->SetBytesPerSecond(rate_bytes_per_sec);
    LOG(INFO) << "Set rate bytes per second of rate limiter to " << rate_bytes_per_sec;
  }

  LOG(INFO) << "Next update hours " << next_update_hours;
  if (next_update_hours != 0) {
    std::chrono::hours now_hours = std::chrono::duration_cast<std::chrono::hours>(now.time_since_epoch());
    std::chrono::system_clock::time_point system_next_update_tp(now_hours + std::chrono::hours(next_update_hours));
    std::chrono::nanoseconds nanosec_later =
        std::chrono::duration_cast<std::chrono::nanoseconds>(system_next_update_tp - now);
    std::chrono::steady_clock::time_point next_update_tp = std::chrono::steady_clock::now() + nanosec_later;

    std::time_t tt_next_update = std::chrono::system_clock::to_time_t(system_next_update_tp);
    LOG(INFO) << "Next rate limit update is scheduled at " << std::put_time(std::localtime(&tt_next_update), "%F %T");
    std::weak_ptr<RocksDbConfigFactory> weak_config_factory = shared_from_this();
    timer_evb_->runInEventBaseThread([weak_config_factory, next_update_tp]() {
      std::shared_ptr<RocksDbConfigFactory> config_factory = weak_config_factory.lock();
      if (!config_factory) {
        return;
      }
      config_factory->timer_evb_->scheduleAt(
          [weak_config_factory]() {
            std::shared_ptr<RocksDbConfigFactory> config_factory = weak_config_factory.lock();
            if (!config_factory) {
              return;
            }
            config_factory->updateRateLimit();
          },
          next_update_tp);
    });
  }
}

uint64 RocksDbConfigFactory::getTableHash(const std::string& db_name, const std::string& table_name) const {
  auto key = CityHash64WithSeed(db_name.c_str(), db_name.length(), 0);
  return CityHash64WithSeed(table_name.c_str(), table_name.length(), key);
}

}  // namespace laser
