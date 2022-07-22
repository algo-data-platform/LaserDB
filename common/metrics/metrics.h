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

#include "folly/Singleton.h"
#include "folly/io/async/AsyncTimeout.h"
#include "folly/io/async/ScopedEventBaseThread.h"
#include "folly/stats/TimeseriesHistogram.h"
#include "folly/SpinLock.h"

#include "city.h"

namespace metrics {

class MetricsThread : public folly::ScopedEventBaseThread {
 public:
  explicit MetricsThread(const std::string& name) : folly::ScopedEventBaseThread(name) {}
};

using MetricTags = folly::Optional<std::unordered_map<std::string, std::string>>;

class MetricBase {
 public:
  MetricBase() = default;
  MetricBase(const std::string module_name, const std::string metric_name, folly::EventBase* evb)
      : module_name_(module_name), metric_name_(metric_name), evb_(evb) {}

  const std::string& getModuleName() { return module_name_; }

  const std::string& getMetricName() { return metric_name_; }

  const std::unordered_map<std::string, std::string>& getTags() { return tags_; }

  void setTags(const MetricTags& tags) {
    if (tags) {
      tags_ = *tags;
    }
  }

  folly::EventBase* getEventBase() { return evb_; }

  ~MetricBase() = default;

 private:
  std::string module_name_;
  std::string metric_name_;
  std::unordered_map<std::string, std::string> tags_;
  folly::EventBase* evb_;
};

class Gauges : public MetricBase {
 public:
  Gauges() = default;
  Gauges(const std::string module_name, const std::string metric_name, uint32_t interval_ms,
         folly::Function<double()> callback, folly::EventBase* evb);
  double getValue();
  void stop();
  ~Gauges();

 private:
  void updateValue();
  uint32_t interval_ms_;
  folly::Function<double()> callback_;
  double value_ = 0;
  std::unique_ptr<folly::AsyncTimeout> timeout_;
};

class TagKey {
 public:
  TagKey(const std::string& tag_key, const std::string& tag_value);
  ~TagKey() = default;
  inline const std::string& getTagKey() const { return tag_key_; }
  inline const std::string& getTagValue() const { return tag_value_; }

 private:
  std::string tag_key_;
  std::string tag_value_;
};

class TagMetric {
 public:
  TagMetric(const std::vector<TagKey>& keys, double value) : keys_(keys), value_(value) {}
  inline const std::vector<TagKey>& getTagKeys() const { return keys_; }
  inline double getValue() { return value_; }
  ~TagMetric() = default;

 private:
  std::vector<TagKey> keys_;
  double value_ = 0.0;
};

class BatchGauges : public MetricBase {
 public:
  BatchGauges() = default;
  BatchGauges(const std::string module_name, const std::string metric_name, uint32_t interval_ms,
              folly::Function<std::vector<TagMetric>()> callback, folly::EventBase* evb);
  std::vector<TagMetric> getValue();
  void stop();
  ~BatchGauges();

 private:
  void updateValue();
  uint32_t interval_ms_;
  folly::Function<std::vector<TagMetric>()> callback_;
  std::vector<TagMetric> value_;
  std::unique_ptr<folly::AsyncTimeout> timeout_;
  folly::SpinLock spinlock_;
};

class Counter : public MetricBase {
 public:
  Counter() = default;
  Counter(const std::string module_name, const std::string metric_name, int precision, folly::EventBase* evb)
      : MetricBase(module_name, metric_name, evb), precision_factor_(std::pow(10, precision)) {}
  double getValue() { return value_.load(std::memory_order_relaxed) / precision_factor_; }
  void inc(double val) { value_.fetch_add(static_cast<int64_t>(val * precision_factor_), std::memory_order_relaxed); }
  void dec(double val) { value_.fetch_sub(static_cast<int64_t>(val * precision_factor_), std::memory_order_relaxed); }
  ~Counter() = default;

 private:
  std::atomic<int64_t> value_{0};
  int precision_factor_ = 0;
};

class Meter : public MetricBase, public std::enable_shared_from_this<Meter> {
 public:
  Meter() = default;
  Meter(const std::string module_name, const std::string metric_name, folly::EventBase* evb,
        const std::vector<int>& time_level_min);

  void init();
  void stop();
  double getCount() { return counter_->getValue(); }
  double getMeanRate() { return mean_rate_; }
  const std::vector<int>& getTimeLevels() { return time_level_min_; }
  double getMinuteRate(int time_level);

  void mark() { counter_->inc(1); }
  void mark(double val) { counter_->inc(val); }

  ~Meter();

 private:
  std::shared_ptr<Counter> counter_;
  std::vector<double> history_;
  std::vector<int> time_level_min_;
  int64_t run_time_ = 0;
  double mean_rate_ = 0.0;
  std::unordered_map<int, double> minutes_rate_;
  std::unique_ptr<folly::AsyncTimeout> timeout_mean_;
  std::unordered_map<int, std::unique_ptr<folly::AsyncTimeout>> timeouts_;
  folly::SpinLock spinlock_;

  void updateMeanValue();
  void updateValue(int time_level);
};

class Histograms : public MetricBase {
 public:
  Histograms() = default;
  Histograms(const std::string module_name, const std::string metric_name, double bucket_size, double min, double max,
             folly::EventBase* evb, const std::vector<int>& time_level_sec);
  void update();
  void addValue(double val);
  double getPercentileEstimate(double pct, int level);
  double getAvg(int level);
  double getCount(int level);
  const std::vector<int> getTimeLevels() { return time_level_sec_; }
  ~Histograms() = default;

 private:
  folly::SpinLock spinlock_;
  std::shared_ptr<folly::TimeseriesHistogram<double, std::chrono::steady_clock>> time_histogram_;
  std::vector<int> time_level_sec_;
};

class Timers : public MetricBase {
 public:
  Timers() = default;
  Timers(const std::string module_name, const std::string metric_name, double bucket_size, double min, double max,
         folly::EventBase* evb, const std::vector<int>& time_level_his_sec,
         const std::vector<int>& time_level_meter_min);
  void update(double time);
  std::shared_ptr<Histograms> getHistograms() { return histograms_; }
  std::shared_ptr<Meter> getMeter() { return meter_; }

 private:
  std::shared_ptr<Histograms> histograms_;
  std::shared_ptr<Meter> meter_;
};

class Timer {
 public:
  explicit Timer(Timers* timers) : timers_(timers) { start_time_ = std::chrono::steady_clock::now(); }
  ~Timer() { stop(); }
  void inline stop() {
    std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span =
        std::chrono::duration_cast<std::chrono::duration<double>>(t2 - start_time_);
    double time = static_cast<double>(time_span.count() * 1000);
    timers_->update(time);
  }

 private:
  std::chrono::steady_clock::time_point start_time_;
  Timers* timers_;
};

template <typename T>
class MetricStore {
 public:
  MetricStore() = default;
  ~MetricStore() = default;

  const std::unordered_map<uint64_t, std::shared_ptr<T>> getAll() {
    std::unordered_map<uint64_t, std::shared_ptr<T>> list;
    metrics_.withRLock([&list](auto& metrics) { list = metrics; });
    return list;
  }

  std::shared_ptr<T> getOne(const std::string& module_name, const std::string& metric_name, MetricTags tags) {
    uint64_t key = metricHash(module_name, metric_name, tags);
    return metrics_.withRLockPtr([key](auto rlock) {
      if (rlock->find(key) != rlock->end()) {
        return rlock->at(key);
      }
      return std::shared_ptr<T>(nullptr);
    });
  }

  using MetricCreator = folly::Function<std::shared_ptr<T>()>;
  std::shared_ptr<T> build(const std::string& module_name, const std::string& metric_name, MetricTags tags,
                           MetricCreator creator) {
    return metrics_.withULockPtr([this, module_name, metric_name, tags, &creator](auto ulock) {
      uint64_t key = metricHash(module_name, metric_name, tags);
      if (ulock->find(key) != ulock->end()) {
        return ulock->at(key);
      }

      auto wlock = ulock.moveFromUpgradeToWrite();
      auto metric = creator();
      metric->setTags(tags);
      (*wlock)[key] = metric;
      return metric;
    });
  }

  uint64 metricHash(const std::string& module_name, const std::string& metric_name, MetricTags tags) {
    uint64_t key = CityHash64WithSeed(module_name.c_str(), module_name.size(), 0);
    key = CityHash64WithSeed(metric_name.c_str(), metric_name.size(), key);
    if (tags) {
      for (auto& t : *tags) {
        key = CityHash64WithSeed(t.first.c_str(), t.first.size(), key);
        key = CityHash64WithSeed(t.second.c_str(), t.second.size(), key);
      }
    }

    return key;
  }

 private:
  folly::Synchronized<std::unordered_map<uint64_t, std::shared_ptr<T>>> metrics_;
};

class Metrics {
 public:
  static std::shared_ptr<Metrics> getInstance();
  Metrics();
  ~Metrics() {
    LOG(ERROR) << "Metrics deleted";
    stop();
  }
  std::shared_ptr<Gauges> buildGauges(const std::string& module_name, const std::string& metric_name,
                                      uint32_t interval_ms, folly::Function<double()> callback,
                                      MetricTags tags = folly::none);
  std::shared_ptr<BatchGauges> buildBatchGauges(const std::string& module_name, const std::string& metric_name,
                                                uint32_t interval_ms,
                                                folly::Function<std::vector<TagMetric>()> callback,
                                                MetricTags tags = folly::none);
  std::shared_ptr<Counter> buildCounter(const std::string& module_name, const std::string& metric_name,
                                        MetricTags tags = folly::none, int precision = 0);
  std::shared_ptr<Meter> buildMeter(const std::string& module_name, const std::string& metric_name,
                                    MetricTags tags = folly::none, const std::vector<int>& time_level_min = {1, 5, 15});
  std::shared_ptr<Histograms> buildHistograms(const std::string& module_name, const std::string& metric_name,
                                              double bucket_size, double min, double max, MetricTags tags = folly::none,
                                              const std::vector<int>& time_level_sec = {3600, 60});
  std::shared_ptr<Timers> buildTimers(const std::string& module_name, const std::string& metric_name,
                                      double bucket_size, double min, double max, MetricTags tags = folly::none,
                                      const std::vector<int>& time_level_his_sec = {3600, 60},
                                      const std::vector<int>& time_level_meter_min = {1, 5, 15});

  const std::unordered_map<uint64_t, std::shared_ptr<Gauges>> getGauges();
  const std::unordered_map<uint64_t, std::shared_ptr<BatchGauges>> getBatchGauges();
  const std::unordered_map<uint64_t, std::shared_ptr<Counter>> getCounters();
  const std::unordered_map<uint64_t, std::shared_ptr<Meter>> getMeters();
  const std::unordered_map<uint64_t, std::shared_ptr<Histograms>> getHistograms();
  const std::unordered_map<uint64_t, std::shared_ptr<Timers>> getTimers();

  std::shared_ptr<Timers> getOneTimer(const std::string& module_name, const std::string& metric_name,
                                      MetricTags tags);
  std::shared_ptr<Meter> getOneMeter(const std::string& module_name, const std::string& metric_name,
                                     MetricTags tags);

  void stop() {
    if (!has_stop_.test_and_set()) {
      for (auto& gauge : getGauges()) {
        gauge.second->stop();
      }
      gauges_.reset();
      for (auto& batch_gauge : getBatchGauges()) {
        batch_gauge.second->stop();
      }
      batch_gauges_.reset();
      for (auto& meter : getMeters()) {
        meter.second->stop();
      }
      meters_.reset();
      counters_.reset();
      histograms_.reset();
      for (auto& timer : getTimers()) {
        timer.second->getMeter()->stop();
      }
      timers_.reset();
    }
  }

 private:
  std::unique_ptr<MetricsThread> metrics_thread_;
  std::shared_ptr<MetricStore<Gauges>> gauges_;
  std::shared_ptr<MetricStore<BatchGauges>> batch_gauges_;
  std::shared_ptr<MetricStore<Meter>> meters_;
  std::shared_ptr<MetricStore<Counter>> counters_;
  std::shared_ptr<MetricStore<Histograms>> histograms_;
  std::shared_ptr<MetricStore<Timers>> timers_;
  folly::EventBase* evb_;
  std::atomic_flag has_stop_ = ATOMIC_FLAG_INIT;
};

}  // namespace metrics
