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

#include "http_client.h"
#include <iostream>
#include "folly/portability/GFlags.h"
#include "folly/GLog.h"

namespace hdfs {

DEFINE_int32(call_hdfs_metadata_connect_timeout, 1000, "connect timeout in milliseconds");
DEFINE_int32(call_hdfs_metadata_client_timeout, 1000, "read/write timeout in milliseconds");
constexpr static char ERROR[] = "error";

ResultStatus HdfsHttpClient::refreshChecksumInfo(const std::string& url, std::string *data,
                                                 std::shared_ptr<metrics::Counter> counter) {
  using HttpResponse = service_framework::http::HttpResponse;
  service_framework::http::HttpOption option(FLAGS_call_hdfs_metadata_connect_timeout,
                                             FLAGS_call_hdfs_metadata_client_timeout);
  folly::Future<std::shared_ptr<HttpResponse>> res = makeGetRequest(url, option);
  const std::shared_ptr<HttpResponse>& response = std::move(res).get();
  if (!response->isOk()) {
    FB_LOG_EVERY_MS(INFO, 100) << "http get response fail.";
    // 连接失败上报告警
    counter->inc(1);
    return ResultStatus::RUN_FAIL;
  }
  // 由于请求结果的不确定性，所以在返回 header 信息，body 信息时通过 folly::Optional 封装
  HttpResponse::OptionalHttpMessage message = response->moveMessage();
  if (!message) {
    FB_LOG_EVERY_MS(INFO, 100) << "Status:" << (*message)->getStatusCode();
  }
  folly::Optional<folly::fbstring> body = response->moveToFbString();
  *data = folly::toStdString(body.value());
  if (*data == ERROR) {
    // 获取失败上报告警
    counter->inc(1);
    FB_LOG_EVERY_MS(INFO, 100) << "get checksum data fail, url : " << url;
    return ResultStatus::RUN_FAIL;
  }

  if (data->length() == 0) {
    // 当前库表数据为空不上报告警
    FB_LOG_EVERY_MS(INFO, 100) << "No such file or directory, url : " << url;
    return ResultStatus::RUN_FAIL;
  }

  return ResultStatus::RUN_SUCC;
}

}  // namespace hdfs
