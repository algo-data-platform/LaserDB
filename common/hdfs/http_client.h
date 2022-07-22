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
 * @author liubang <it.liubang@gmail.com>
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#pragma once

#include "hdfs.h"
#include "proxygen/lib/http/HTTPMethod.h"
#include "common/metrics/metrics.h"
#include "common/http/http_client_manager.h"

namespace hdfs {

class HdfsHttpClient {
 public:
  HdfsHttpClient() = default;
  ~HdfsHttpClient() = default;
  ResultStatus refreshChecksumInfo(const std::string& url, std::string *data,
                                   std::shared_ptr<metrics::Counter> counter);
};

}  // namespace hdfs
