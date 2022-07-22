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

#include "folly/dynamic.h"
#include "folly/String.h"

#include "city.h"
#include "common/metrics/metrics.h"

namespace metrics {

class TransformerInterface {
 public:
  TransformerInterface(const std::string& host, uint16_t port, const std::string& service_name)
      : host_(host), port_(port), service_name_(service_name) {
    metrics_ = metrics::Metrics::getInstance();
  }
  virtual ~TransformerInterface() = default;
  virtual const std::string transform() = 0;

 protected:
  std::string host_;
  uint16_t port_;
  std::string service_name_;
  std::shared_ptr<metrics::Metrics> metrics_;
  std::vector<double> percentiles_{0.999, 0.99, 0.98, 0.95, 0.75, 0.5};
};

class JsonTransformer : public TransformerInterface {
 public:
  JsonTransformer(const std::string& host, uint16_t port, const std::string& service_name,
                  const std::string& service_status)
      : TransformerInterface(host, port, service_name),
        service_status_(service_status) {}
  ~JsonTransformer() = default;
  const std::string transform() override;

 private:
  void transformGauges(folly::dynamic* result);
  void transformBatchGauges(folly::dynamic* result);
  void transformCounters(folly::dynamic* result);
  void transformMeters(folly::dynamic* result);
  void transformHistograms(folly::dynamic* result);
  void transformTimers(folly::dynamic* result);
  folly::dynamic& getMetricValues(folly::dynamic* result, const std::shared_ptr<MetricBase>& metric);
  const folly::dynamic getTags(const std::shared_ptr<MetricBase>& metric);
  const std::string getTagsString(const std::shared_ptr<MetricBase>& metric);
  const folly::dynamic getTags(const std::unordered_map<std::string, std::string>& tags);
  const std::string getTagsString(const std::unordered_map<std::string, std::string>& tags);
  void getMeter(folly::dynamic* metric_value, const std::shared_ptr<Meter>& meter);
  void getHistograms(folly::dynamic* metric_value, const std::shared_ptr<Histograms>& histograms);

 private:
  std::string service_status_;
};

// TODO(changyu): Somehow, using unordered_map here is causing fisher coredump. Might be related to the fact that we are
// mixing gcc 4.8 and 7.3 when compiling, which makes some function use different allocators when allocating and
// deallocating memory. Will need to investigate it later.
using PrometheusTag = std::map<std::string, std::string>;
class PrometheusTransformer : public TransformerInterface {
 public:
  PrometheusTransformer(const std::string& host, uint16_t port, const std::string& service_name)
      : TransformerInterface(host, port, service_name) {}
  ~PrometheusTransformer() = default;
  const std::string transform() override;

 private:
  void transformGauges(std::string* result);
  void transformBatchGauges(std::string* result);
  void transformCounters(std::string* result);
  void transformMeters(std::string* result);
  void transformHistograms(std::string* result);
  void transformTimers(std::string* result);
  const std::string getMetricKey(const std::shared_ptr<MetricBase>& metric);
  const PrometheusTag getTags(const std::shared_ptr<MetricBase>& metric);
  const PrometheusTag getTags(const std::unordered_map<std::string, std::string>& tags);
  const std::string formatMetric(const std::string& key, double value, const PrometheusTag& tags);
  void getMeter(std::string* result, const std::shared_ptr<Meter>& meter, const PrometheusTag& public_tags,
                const std::string& tag_key);
  void getHistograms(std::string* result, const std::shared_ptr<Histograms>& histograms,
                     const PrometheusTag& public_tags, const std::string& tag_key);
};

}  // namespace metrics
