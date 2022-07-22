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

#include "metrics.h"

namespace metrics {

folly::Singleton<Metrics> global_service_status = folly::Singleton<Metrics>().shouldEagerInit();
std::shared_ptr<Metrics> Metrics::getInstance() { return global_service_status.try_get(); }

Metrics::Metrics() {
  metrics_thread_ = std::make_unique<MetricsThread>("metrics_thread");
  gauges_ = std::make_shared<MetricStore<Gauges>>();
  batch_gauges_ = std::make_shared<MetricStore<BatchGauges>>();
  meters_ = std::make_shared<MetricStore<Meter>>();
  counters_ = std::make_shared<MetricStore<Counter>>();
  histograms_ = std::make_shared<MetricStore<Histograms>>();
  timers_ = std::make_shared<MetricStore<Timers>>();
  evb_ = metrics_thread_->getEventBase();
}

std::shared_ptr<Gauges> Metrics::buildGauges(const std::string& module_name, const std::string& metric_name,
                                             uint32_t interval, folly::Function<double()> callback, MetricTags tags) {
  return gauges_->build(module_name, metric_name, tags, [this, module_name, metric_name, interval, &callback]() {
    return std::make_shared<Gauges>(module_name, metric_name, interval, std::move(callback), evb_);
  });
}

const std::unordered_map<uint64_t, std::shared_ptr<Gauges>> Metrics::getGauges() { return gauges_->getAll(); }

std::shared_ptr<BatchGauges> Metrics::buildBatchGauges(const std::string& module_name, const std::string& metric_name,
                                                       uint32_t interval,
                                                       folly::Function<std::vector<TagMetric>()> callback,
                                                       MetricTags tags) {
  return batch_gauges_->build(module_name, metric_name, tags, [this, module_name, metric_name, interval, &callback]() {
    return std::make_shared<BatchGauges>(module_name, metric_name, interval, std::move(callback), evb_);
  });
}

const std::unordered_map<uint64_t, std::shared_ptr<BatchGauges>> Metrics::getBatchGauges() {
  return batch_gauges_->getAll();
}

std::shared_ptr<Counter> Metrics::buildCounter(const std::string& module_name, const std::string& metric_name,
                                               MetricTags tags, int precision) {
  return counters_->build(module_name, metric_name, tags, [this, module_name, metric_name, precision]() {
    return std::make_shared<Counter>(module_name, metric_name, precision, evb_);
  });
}

const std::unordered_map<uint64_t, std::shared_ptr<Counter>> Metrics::getCounters() { return counters_->getAll(); }

std::shared_ptr<Meter> Metrics::buildMeter(const std::string& module_name, const std::string& metric_name,
                                           MetricTags tags, const std::vector<int>& time_level_min) {
  return meters_->build(module_name, metric_name, tags, [this, module_name, metric_name, &time_level_min]() {
    auto meter = std::make_shared<Meter>(module_name, metric_name, evb_, time_level_min);
    meter->init();
    return meter;
  });
}

const std::unordered_map<uint64_t, std::shared_ptr<Meter>> Metrics::getMeters() { return meters_->getAll(); }

std::shared_ptr<Meter> Metrics::getOneMeter(
  const std::string& module_name, const std::string& metric_name,
  MetricTags tags) {
  return meters_->getOne(module_name, metric_name, tags);
}

std::shared_ptr<Histograms> Metrics::buildHistograms(const std::string& module_name, const std::string& metric_name,
                                                     double bucket_size, double min, double max, MetricTags tags,
                                                     const std::vector<int>& time_level_sec) {
  return histograms_->build(
      module_name, metric_name, tags, [this, module_name, metric_name, bucket_size, min, max, &time_level_sec]() {
        return std::make_shared<Histograms>(module_name, metric_name, bucket_size, min, max, evb_, time_level_sec);
      });
}

const std::unordered_map<uint64_t, std::shared_ptr<Histograms>> Metrics::getHistograms() {
  return histograms_->getAll();
}

std::shared_ptr<Timers> Metrics::buildTimers(const std::string& module_name, const std::string& metric_name,
                                             double bucket_size, double min, double max, MetricTags tags,
                                             const std::vector<int>& time_level_his_sec,
                                             const std::vector<int>& time_level_meter_min) {
  return timers_->build(
      module_name, metric_name, tags,
      [this, module_name, metric_name, bucket_size, min, max, &time_level_his_sec, &time_level_meter_min]() {
        return std::make_shared<Timers>(module_name, metric_name, bucket_size, min, max, evb_, time_level_his_sec,
                                        time_level_meter_min);
      });
}

const std::unordered_map<uint64_t, std::shared_ptr<Timers>> Metrics::getTimers() { return timers_->getAll(); }

std::shared_ptr<Timers> Metrics::getOneTimer(
  const std::string& module_name, const std::string& metric_name,
  MetricTags tags) {
  return timers_->getOne(module_name, metric_name, tags);
}

Gauges::Gauges(const std::string module_name, const std::string metric_name, uint32_t interval_ms,
               folly::Function<double()> callback, folly::EventBase* evb)
    : MetricBase(module_name, metric_name, evb), interval_ms_(interval_ms), callback_(std::move(callback)) {
  timeout_ = folly::AsyncTimeout::make(*getEventBase(), [this]() noexcept { updateValue(); });
  updateValue();
}

double Gauges::getValue() { return value_; }

void Gauges::updateValue() {
  getEventBase()->runInEventBaseThread(
      [this]() { timeout_->scheduleTimeout(std::chrono::milliseconds(interval_ms_)); });

  value_ = callback_();
}

void Gauges::stop() {
  getEventBase()->runInEventBaseThreadAndWait([this]() { timeout_->cancelTimeout(); });
}

Gauges::~Gauges() {}

TagKey::TagKey(const std::string& tag_key, const std::string& tag_value) : tag_key_(tag_key), tag_value_(tag_value) {}

BatchGauges::BatchGauges(const std::string module_name, const std::string metric_name, uint32_t interval_ms,
                         folly::Function<std::vector<TagMetric>()> callback, folly::EventBase* evb)
    : MetricBase(module_name, metric_name, evb), interval_ms_(interval_ms), callback_(std::move(callback)) {
  timeout_ = folly::AsyncTimeout::make(*getEventBase(), [this]() noexcept { updateValue(); });
  updateValue();
}

std::vector<TagMetric> BatchGauges::getValue() {
  folly::SpinLockGuard g(spinlock_);
  return value_;
}

void BatchGauges::updateValue() {
  getEventBase()->runInEventBaseThread(
      [this]() { timeout_->scheduleTimeout(std::chrono::milliseconds(interval_ms_)); });

  auto tempValues = callback_();
  folly::SpinLockGuard g(spinlock_);
  value_ = tempValues;
}

void BatchGauges::stop() {
  getEventBase()->runInEventBaseThreadAndWait([this]() { timeout_->cancelTimeout(); });
}

BatchGauges::~BatchGauges() {}

constexpr static int TIME_LEVEL_ONE_MIN = 1;

Meter::Meter(const std::string module_name, const std::string metric_name, folly::EventBase* evb,
             const std::vector<int>& time_level_min)
    : MetricBase(module_name, metric_name, evb), time_level_min_(time_level_min) {
  counter_ = std::make_shared<Counter>(module_name, metric_name, 0, evb);

  // 由于 1mintue 是一个 cut 单元，所以需要特殊处理, 并且必须存在
  auto has_one_minute = std::find(time_level_min_.begin(), time_level_min_.end(), TIME_LEVEL_ONE_MIN);
  if (has_one_minute == time_level_min_.end()) {
    time_level_min_.push_back(TIME_LEVEL_ONE_MIN);
  }
}

void Meter::init() {
  std::weak_ptr<Meter> weak_meter = shared_from_this();
  timeout_mean_ = folly::AsyncTimeout::make(*getEventBase(), [weak_meter]() noexcept {
    auto meter = weak_meter.lock();
    if (!meter) {
      return;
    }
    meter->updateMeanValue();
  });
  updateMeanValue();

  for (int minute : time_level_min_) {
    {
      folly::SpinLockGuard g(spinlock_);
      timeouts_[minute] = folly::AsyncTimeout::make(*getEventBase(), [ weak_meter, minute ]() noexcept {
        auto meter = weak_meter.lock();
        if (!meter) {
          return;
        }
        meter->updateValue(minute);
      });
    }
    updateValue(minute);
  }
}

void Meter::updateMeanValue() {
  std::weak_ptr<Meter> weak_meter = shared_from_this();
  getEventBase()->runInEventBaseThread([weak_meter]() {
    auto meter = weak_meter.lock();
    if (!meter) {
      return;
    }
    folly::SpinLockGuard g(meter->spinlock_);
    if (meter->timeout_mean_) {
      meter->timeout_mean_->scheduleTimeout(std::chrono::seconds(1));
    }
  });

  run_time_++;
  double mean_rate = (static_cast<double>(counter_->getValue()) / run_time_);
  mean_rate_ = static_cast<double>(static_cast<uint64_t>(mean_rate * 1000)) / 1000;
}

void Meter::updateValue(int time_level) {
  std::weak_ptr<Meter> weak_meter = shared_from_this();
  getEventBase()->runInEventBaseThread([time_level, weak_meter]() {
    auto meter = weak_meter.lock();
    if (!meter) {
      return;
    }
    folly::SpinLockGuard g(meter->spinlock_);
    if (meter->timeouts_[time_level]) {
      meter->timeouts_[time_level]->scheduleTimeout(std::chrono::seconds(60 * time_level));
    }
  });

  if (time_level == TIME_LEVEL_ONE_MIN) {
    history_.insert(history_.begin(), counter_->getValue());
    auto max_time_level = std::max_element(time_level_min_.begin(), time_level_min_.end());
    if (history_.size() > static_cast<size_t>(*max_time_level + 1)) {
      history_.pop_back();
    }
  }

  if (history_.size() >= static_cast<size_t>(time_level + 1)) {
    double minute_rate = static_cast<double>(history_[0] - history_[time_level]) / (60 * time_level);
    folly::SpinLockGuard g(spinlock_);
    minutes_rate_[time_level] = static_cast<double>(static_cast<int64_t>(minute_rate * 1000)) / 1000;
  }
}

double Meter::getMinuteRate(int time_level) {
  folly::SpinLockGuard g(spinlock_);
  if (minutes_rate_.find(time_level) == minutes_rate_.end()) {
    return 0.0;
  }

  return minutes_rate_[time_level];
}

void Meter::stop() {
  getEventBase()->runInEventBaseThreadAndWait([this]() {
    timeout_mean_->cancelTimeout();
    folly::SpinLockGuard g(spinlock_);
    for (auto& t : timeouts_) {
      t.second->cancelTimeout();
      t.second.reset();
    }
    timeout_mean_.reset();
  });
}

Meter::~Meter() {}

using StatsClock = std::chrono::steady_clock;
StatsClock::time_point mkTimePoint() { return std::chrono::steady_clock::now(); }

constexpr static int TIME_LEVEL_ALL_TIME = 0;

Histograms::Histograms(const std::string module_name, const std::string metric_name, double bucket_size, double min,
                       double max, folly::EventBase* evb, const std::vector<int>& time_level_sec)
    : MetricBase(module_name, metric_name, evb), time_level_sec_(time_level_sec) {
  // seconds(0) 代表所有时间段聚合的直方图
  // 并且必须放到数组最后面
  auto has_one_zero = std::find(time_level_sec_.begin(), time_level_sec_.end(), TIME_LEVEL_ALL_TIME);
  if (has_one_zero != time_level_sec_.end()) {
    time_level_sec_.erase(time_level_sec_.begin() + std::distance(time_level_sec_.begin(), has_one_zero));
  }
  std::sort(time_level_sec_.begin(), time_level_sec_.end());
  time_level_sec_.push_back(TIME_LEVEL_ALL_TIME);

  std::vector<std::chrono::steady_clock::duration> durations;
  for (int i = 0; i < time_level_sec_.size(); i++) {
    durations.push_back(std::chrono::seconds(time_level_sec_[i]));
  }

  auto level_time = folly::MultiLevelTimeSeries<double, std::chrono::steady_clock>(
      60, static_cast<int>(time_level_sec_.size()), &durations[0]);
  time_histogram_ = std::make_shared<folly::TimeseriesHistogram<double, std::chrono::steady_clock>>(bucket_size, min,
                                                                                                    max, level_time);
}

void Histograms::addValue(double val) {
  folly::SpinLockGuard g(spinlock_);
  time_histogram_->addValue(mkTimePoint(), val);
}

void Histograms::update() {
  folly::SpinLockGuard g(spinlock_);
  time_histogram_->update(mkTimePoint());
}

double Histograms::getPercentileEstimate(double pct, int level) {
  folly::SpinLockGuard g(spinlock_);
  return time_histogram_->getPercentileEstimate(pct, static_cast<size_t>(level));
}

double Histograms::getAvg(int level) {
  folly::SpinLockGuard g(spinlock_);
  return time_histogram_->avg(static_cast<size_t>(level));
}

double Histograms::getCount(int level) {
  folly::SpinLockGuard g(spinlock_);
  return time_histogram_->count(static_cast<size_t>(level));
}

Timers::Timers(const std::string module_name, const std::string metric_name, double bucket_size, double min, double max,
               folly::EventBase* evb, const std::vector<int>& time_level_his_sec,
               const std::vector<int>& time_level_meter_min)
    : MetricBase(module_name, metric_name, evb) {
  histograms_ = std::make_shared<Histograms>(module_name, metric_name, bucket_size, min, max, evb, time_level_his_sec);
  meter_ = std::make_shared<Meter>(module_name, metric_name, evb, time_level_meter_min);
  meter_->init();
}

void Timers::update(double time) {
  histograms_->addValue(time);
  meter_->mark();
}

}  // namespace metrics
