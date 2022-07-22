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

#include "geo_service.h"

DEFINE_string(db_path, "/tmp/rocksdb_test", "rocksdb test db path");
DEFINE_int32(sleep_ms, 50, "Geo Server start sleep ms");

namespace geo {

GeoService::GeoService() {
  ip_search_requests_ = metrics::Metrics::getInstance()->buildTimers("service", "ip_search_request", 1, 0, 1000);
}

void GeoService::ipSearch(IpSearchResponse& result, std::unique_ptr<IpSearchRequest> req) {
  metrics::Timer timer(ip_search_requests_.get());
  std::this_thread::sleep_for(std::chrono::milliseconds(FLAGS_sleep_ms));
  result.set_code(req->get_ip());
  result.set_country("中国");
  result.set_province("北京");
  result.set_city("北京");
}
}  // geo
