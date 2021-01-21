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

#include <cassert>

#include "geo/if/gen-cpp2/GeoService.h"
#include "common/metrics/metrics.h"

namespace geo {

class GeoService : virtual public GeoServiceSvIf {
 public:
  GeoService();
  ~GeoService() = default;
  void ipSearch(IpSearchResponse& result, std::unique_ptr<IpSearchRequest> req) override;
 private:
  std::shared_ptr<metrics::Timers> ip_search_requests_;
};

}  // geo
