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

#include "common/util.h"
#include "common/metrics/transform.h"
#include "common/version.h"
#include "boost/regex.hpp"
#include "boost/algorithm/string/trim.hpp"

namespace metrics {

constexpr static char MINUTE_KEY_PREFIX[] = "min_";
constexpr static char PERCENT_KEY_PREFIX[] = "percent_";
constexpr static char MEAN_RATE_KEY[] = "mean_rate";
constexpr static char COUNT_KEY[] = "count";
constexpr static char AVG_KEY[] = "avg";

constexpr static char JSON_TAGS_KEY[] = "tags";
constexpr static char JSON_VALUE_KEY[] = "value";
constexpr static char JSON_TAGS_KV_JOIN_TOKEN[] = "=";
constexpr static char JSON_TAGS_JOIN_TOKEN[] = ",";
constexpr static char JSON_TAGS_JOIN_DEFAULT[] = "default";
constexpr static char JSON_SERVICE_BASE_MODULE_NAME[] = "service_base";
constexpr static char JSON_SERVICE_BASE_SERVICE_PORT[] = "port";
constexpr static char JSON_SERVICE_BASE_SERVICE_HOST[] = "host";
constexpr static char JSON_SERVICE_BASE_SERVICE_NAME[] = "service_name";
constexpr static char JSON_SERVICE_BASE_SERVICE_STATUS[] = "service_status";
constexpr static char JSON_SERVICE_BASE_SERVICE_PACKAGE_TAG[] = "package_tag";

constexpr static char PROMETHEUS_KEY_PREFIX[] = "ad_core_";
constexpr static char PROMETHEUS_SERVICE_ADDRESS_KEY[] = "service_address";
constexpr static char PROMETHEUS_SERVICE_NAME_KEY[] = "service_name";
constexpr static char PROMETHEUS_METRIC_TYPE[] = "metric_type";
constexpr static char PROMETHEUS_METRIC_TYPE_TIMERS[] = "timers";
constexpr static char PROMETHEUS_METRIC_TYPE_HISTOGRAMS[] = "histograms";
constexpr static char PROMETHEUS_METRIC_HISTOGRAMS_LEVEL[] = "level";
constexpr static char PROMETHEUS_METRIC_TYPE_COUNTERS[] = "counters";
constexpr static char PROMETHEUS_METRIC_TYPE_GAUGES[] = "gauges";
constexpr static char PROMETHEUS_METRIC_TYPE_METERS[] = "meters";
constexpr static char PROMETHEUS_HISTOGRAMS_TYPE[] = "histograms_type";
constexpr static char PROMETHEUS_TIMERS_TYPE[] = "timers_type";
constexpr static char PROMETHEUS_METER_TYPE[] = "meter_type";

const std::string JsonTransformer::transform() {
  folly::dynamic status_map = folly::dynamic::object;

  transformGauges(&status_map);
  transformBatchGauges(&status_map);
  transformCounters(&status_map);
  transformMeters(&status_map);
  transformHistograms(&status_map);
  transformTimers(&status_map);

  // service info
  folly::dynamic service_base = folly::dynamic::object;
  service_base.insert(JSON_SERVICE_BASE_SERVICE_HOST, host_);
  service_base.insert(JSON_SERVICE_BASE_SERVICE_PORT, port_);
  service_base.insert(JSON_SERVICE_BASE_SERVICE_NAME, service_name_);
  service_base.insert(JSON_SERVICE_BASE_SERVICE_STATUS, service_status_);
  service_base.insert(JSON_SERVICE_BASE_SERVICE_PACKAGE_TAG, PACKAGE_TAG);
  status_map.insert(JSON_SERVICE_BASE_MODULE_NAME, service_base);

  std::string result;
  common::toJson(&result, status_map);
  return result;
}

const folly::dynamic JsonTransformer::getTags(const std::shared_ptr<MetricBase>& metric) {
  auto tags = metric->getTags();
  return getTags(tags);
}

const folly::dynamic JsonTransformer::getTags(const std::unordered_map<std::string, std::string>& tags) {
  folly::dynamic dy_tags = folly::dynamic::object;
  for (auto& tag : tags) {
    dy_tags.insert(tag.first, tag.second);
  }
  return dy_tags;
}

const std::string JsonTransformer::getTagsString(const std::shared_ptr<MetricBase>& metric) {
  std::string result;
  auto tags = metric->getTags();
  return getTagsString(tags);
}

const std::string JsonTransformer::getTagsString(const std::unordered_map<std::string, std::string>& tags) {
  std::string result;
  std::vector<std::string> kv;
  for (auto& tag : tags) {
    kv.push_back(folly::to<std::string>(tag.first, JSON_TAGS_KV_JOIN_TOKEN, tag.second));
  }

  folly::join(JSON_TAGS_JOIN_TOKEN, kv, result);

  if (result.empty()) {
    result = JSON_TAGS_JOIN_DEFAULT;
  }
  return result;
}

folly::dynamic& JsonTransformer::getMetricValues(folly::dynamic* result, const std::shared_ptr<MetricBase>& metric) {
  std::string module_name = metric->getModuleName();
  std::string metric_name = metric->getMetricName();
  if (result->find(module_name) == result->items().end()) {
    folly::dynamic module_values = folly::dynamic::object;
    result->insert(module_name, module_values);
  }
  folly::dynamic& module_map = result->at(module_name);
  if (module_map.find(metric_name) == module_map.items().end()) {
    folly::dynamic metric_values = folly::dynamic::object;
    module_map.insert(metric_name, metric_values);
  }
  folly::dynamic& metric_values = module_map.at(metric_name);
  return metric_values;
}

void JsonTransformer::transformGauges(folly::dynamic* result) {
  auto gauges = metrics_->getGauges();
  for (auto& t : gauges) {
    folly::dynamic metric_value = folly::dynamic::object;
    metric_value[JSON_TAGS_KEY] = getTags(t.second);
    metric_value[JSON_VALUE_KEY] = t.second->getValue();
    folly::dynamic& metric_values = getMetricValues(result, t.second);
    metric_values.insert(getTagsString(t.second), metric_value);
  }
}

void JsonTransformer::transformBatchGauges(folly::dynamic* result) {
  auto gauges = metrics_->getBatchGauges();
  for (auto& t : gauges) {
    auto origin_tags = t.second->getTags();
    auto tag_metrics = t.second->getValue();
    for (auto& tag_metric : tag_metrics) {
      folly::dynamic metric_value = folly::dynamic::object;
      auto temp_tags = origin_tags;
      for (auto& key : tag_metric.getTagKeys()) {
        temp_tags.insert({key.getTagKey(), key.getTagValue()});
      }
      metric_value[JSON_TAGS_KEY] = getTags(temp_tags);
      metric_value[JSON_VALUE_KEY] = tag_metric.getValue();
      folly::dynamic& metric_values = getMetricValues(result, t.second);
      metric_values.insert(getTagsString(temp_tags), metric_value);
    }
  }
}

void JsonTransformer::transformCounters(folly::dynamic* result) {
  auto counters = metrics_->getCounters();
  for (auto& t : counters) {
    folly::dynamic metric_value = folly::dynamic::object;
    metric_value[JSON_TAGS_KEY] = getTags(t.second);
    metric_value[JSON_VALUE_KEY] = t.second->getValue();
    folly::dynamic& metric_values = getMetricValues(result, t.second);
    metric_values.insert(getTagsString(t.second), metric_value);
  }
}

void JsonTransformer::transformMeters(folly::dynamic* result) {
  auto meters = metrics_->getMeters();
  for (auto& t : meters) {
    folly::dynamic metric_value = folly::dynamic::object;
    metric_value[JSON_TAGS_KEY] = getTags(t.second);
    getMeter(&metric_value, t.second);
    folly::dynamic& metric_values = getMetricValues(result, t.second);
    metric_values.insert(getTagsString(t.second), metric_value);
  }
}

void JsonTransformer::transformHistograms(folly::dynamic* result) {
  auto histograms = metrics_->getHistograms();
  for (auto& t : histograms) {
    folly::dynamic metric_value = folly::dynamic::object;
    metric_value[JSON_TAGS_KEY] = getTags(t.second);
    getHistograms(&metric_value, t.second);
    folly::dynamic& metric_values = getMetricValues(result, t.second);
    metric_values.insert(getTagsString(t.second), metric_value);
  }
}

void JsonTransformer::transformTimers(folly::dynamic* result) {
  auto timers = metrics_->getTimers();
  for (auto& t : timers) {
    folly::dynamic metric_value = folly::dynamic::object;
    metric_value[JSON_TAGS_KEY] = getTags(t.second);
    getHistograms(&metric_value, t.second->getHistograms());
    getMeter(&metric_value, t.second->getMeter());
    folly::dynamic& metric_values = getMetricValues(result, t.second);
    metric_values.insert(getTagsString(t.second), metric_value);
  }
}

void JsonTransformer::getMeter(folly::dynamic* metric_value, const std::shared_ptr<Meter>& meter) {
  std::vector<int> levels = meter->getTimeLevels();
  for (int level : levels) {
    (*metric_value)[folly::to<std::string>(MINUTE_KEY_PREFIX, level)] = meter->getMinuteRate(level);
  }
  (*metric_value)[MEAN_RATE_KEY] = meter->getMeanRate();
  (*metric_value)[COUNT_KEY] = meter->getCount();
}

void JsonTransformer::getHistograms(folly::dynamic* metric_value, const std::shared_ptr<Histograms>& histograms) {
  std::vector<int> levels = histograms->getTimeLevels();
  int all_time_level = levels.size() - 1;

  histograms->update();
  for (double percent : percentiles_) {
    (*metric_value)[folly::to<std::string>(PERCENT_KEY_PREFIX, percent)] =
        histograms->getPercentileEstimate(percent * 100, all_time_level);
  }
  (*metric_value)[AVG_KEY] = histograms->getAvg(all_time_level);
}

const std::string PrometheusTransformer::transform() {
  std::string result;
  transformGauges(&result);
  transformBatchGauges(&result);
  transformCounters(&result);
  transformMeters(&result);
  transformHistograms(&result);
  transformTimers(&result);
  return result;
}

const PrometheusTag PrometheusTransformer::getTags(const std::shared_ptr<MetricBase>& metric) {
  return getTags(metric->getTags());
}

const PrometheusTag PrometheusTransformer::getTags(const std::unordered_map<std::string, std::string>& metric_tags) {
  PrometheusTag tags;
  tags[PROMETHEUS_SERVICE_ADDRESS_KEY] = folly::to<std::string>(host_, ":", port_);
  tags[PROMETHEUS_SERVICE_NAME_KEY] = service_name_;
  for (auto& t : metric_tags) {
    tags[t.first] = t.second;
  }
  return tags;
}

const std::string PrometheusTransformer::getMetricKey(const std::shared_ptr<MetricBase>& metric) {
  std::string key = folly::to<std::string>(metric->getModuleName(), "_", metric->getMetricName());
  return boost::regex_replace(key, boost::regex("\\\.|\\\-"), "_");
}

void PrometheusTransformer::transformGauges(std::string* result) {
  auto gauges = metrics_->getGauges();
  for (auto& t : gauges) {
    PrometheusTag tags = getTags(t.second);
    tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_GAUGES;
    *result += formatMetric(getMetricKey(t.second), t.second->getValue(), tags);
  }
}

void PrometheusTransformer::transformBatchGauges(std::string* result) {
  auto gauges = metrics_->getBatchGauges();
  for (auto& t : gauges) {
    auto origin_tags = t.second->getTags();
    auto tag_metrics = t.second->getValue();
    for (auto& tag_metric : tag_metrics) {
      auto temp_tags = origin_tags;
      for (auto& key : tag_metric.getTagKeys()) {
        temp_tags.insert({key.getTagKey(), key.getTagValue()});
      }

      PrometheusTag tags = getTags(temp_tags);
      tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_GAUGES;
      *result += formatMetric(getMetricKey(t.second), tag_metric.getValue(), tags);
    }
  }
}

void PrometheusTransformer::transformCounters(std::string* result) {
  auto counters = metrics_->getCounters();
  for (auto& t : counters) {
    PrometheusTag tags = getTags(t.second);
    tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_COUNTERS;
    *result += formatMetric(getMetricKey(t.second), t.second->getValue(), tags);
  }
}

void PrometheusTransformer::transformMeters(std::string* result) {
  auto meters = metrics_->getMeters();
  for (auto& t : meters) {
    PrometheusTag tags = getTags(t.second);
    tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_METERS;
    getMeter(result, t.second, tags, PROMETHEUS_METER_TYPE);
  }
}

void PrometheusTransformer::transformHistograms(std::string* result) {
  auto histograms = metrics_->getHistograms();
  for (auto& t : histograms) {
    PrometheusTag tags = getTags(t.second);
    tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_HISTOGRAMS;
    getHistograms(result, t.second, tags, PROMETHEUS_HISTOGRAMS_TYPE);
  }
}

void PrometheusTransformer::transformTimers(std::string* result) {
  auto timers = metrics_->getTimers();
  for (auto& t : timers) {
    PrometheusTag tags = getTags(t.second);
    tags[PROMETHEUS_METRIC_TYPE] = PROMETHEUS_METRIC_TYPE_TIMERS;
    getHistograms(result, t.second->getHistograms(), tags, PROMETHEUS_TIMERS_TYPE);
    getMeter(result, t.second->getMeter(), tags, PROMETHEUS_TIMERS_TYPE);
  }
}

void PrometheusTransformer::getMeter(std::string* result, const std::shared_ptr<Meter>& meter,
                                     const PrometheusTag& public_tags, const std::string& tag_key) {
  PrometheusTag tags = public_tags;
  std::vector<int> levels = meter->getTimeLevels();
  for (int level : levels) {
    tags[tag_key] = folly::to<std::string>(MINUTE_KEY_PREFIX, level);
    *result += formatMetric(getMetricKey(meter), meter->getMinuteRate(level), tags);
  }
  tags[tag_key] = MEAN_RATE_KEY;
  *result += formatMetric(getMetricKey(meter), meter->getMeanRate(), tags);
  tags[tag_key] = COUNT_KEY;
  *result += formatMetric(getMetricKey(meter), meter->getCount(), tags);
}

void PrometheusTransformer::getHistograms(std::string* result, const std::shared_ptr<Histograms>& histograms,
                                          const PrometheusTag& public_tags, const std::string& tag_key) {
  PrometheusTag tags = public_tags;
  std::vector<int> levels = histograms->getTimeLevels();

  histograms->update();
  for (size_t i = 0; i < levels.size(); i++) {
    for (double percent : percentiles_) {
      tags[tag_key] = folly::to<std::string>(PERCENT_KEY_PREFIX, percent);
      tags[PROMETHEUS_METRIC_HISTOGRAMS_LEVEL] = folly::to<std::string>(levels[i]);
      *result += formatMetric(getMetricKey(histograms), histograms->getPercentileEstimate(percent * 100, i), tags);
    }
    tags[tag_key] = AVG_KEY;
    *result += formatMetric(getMetricKey(histograms), histograms->getAvg(i), tags);
  }
}

const std::string PrometheusTransformer::formatMetric(const std::string& key, double value, const PrometheusTag& tags) {
  std::string result = folly::to<std::string>(PROMETHEUS_KEY_PREFIX, key);

  if (!tags.empty()) {
    result += "{";
    for (auto& t : tags) {
      result += folly::to<std::string>(t.first, "=\"", t.second, "\",");
    }
    boost::algorithm::trim_right_if(result, boost::algorithm::is_any_of(","));
    result += "} ";
  }

  return folly::to<std::string>(result, value, "\n");
}

}  // namespace metrics
