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

#pragma once

#include <string>
#include <vector>

#include "folly/dynamic.h"

#include "common/util.h"
#include "raw_client.h"
#include "params.h"
#include "entity.h"

namespace service_framework {
namespace consul {

class KVClient : protected RawClient {
 public:
  KVClient(const std::string& host, int port) : RawClient(host, port) {}
  KVClient(const std::string& host, int port, int connect_timeout, int socket_timeout)
      : RawClient(host, port, connect_timeout, socket_timeout) {}
  ~KVClient() = default;

  static const QueryParams defaultQueryParams() {
    QueryParams query_params;
    return query_params;
  }

  static const PutParams defaultPutParams() {
    PutParams put_params;
    return put_params;
  }

  // sync request
  folly::Optional<ConsulResponse<bool>> setValueSync(const std::string& key, const std::string& value,
                                                     const std::string& token = "",
                                                     const QueryParams& query_params = defaultQueryParams(),
                                                     const PutParams& put_params = defaultPutParams());

  folly::Optional<ConsulResponse<Value>> getValueSync(const std::string& key, const std::string& token = "",
                                                      const QueryParams& query_params = defaultQueryParams());

  folly::Optional<ConsulResponse<std::vector<Value>>> getValuesSync(const std::string& key_prefix,
                                                                    const std::string& token = "",
                                                                    const QueryParams& query_params =
                                                                        defaultQueryParams());

  folly::Optional<ConsulResponse<bool>> deleteValueSync(const std::string& key, const std::string& token = "",
                                                        const QueryParams& query_params = defaultQueryParams());

  folly::Optional<ConsulResponse<bool>> deleteValuesSync(const std::string& key_prefix, const std::string& token = "",
                                                         const QueryParams& query_params = defaultQueryParams());

  // async request
  folly::Future<RawClient::HttpResponse> getValue(const std::string& key, const std::string& token = "",
                                                  const QueryParams& query_params = defaultQueryParams());

  folly::Future<RawClient::HttpResponse> getValues(const std::string& key_prefix, const std::string& token = "",
                                                   const QueryParams& query_params = defaultQueryParams());

  folly::Future<RawClient::HttpResponse> setValue(const std::string& key, const std::string& value,
                                                  const std::string& token = "",
                                                  const QueryParams& query_params = defaultQueryParams(),
                                                  const PutParams& put_params = defaultPutParams());

  folly::Future<RawClient::HttpResponse> deleteValue(const std::string& key, const std::string& token = "",
                                                     const QueryParams& query_params = defaultQueryParams());

  folly::Future<RawClient::HttpResponse> deleteValues(const std::string& key_prefix, const std::string& token = "",
                                                      const QueryParams& query_params = defaultQueryParams());

  // process responses methods
  folly::Optional<ConsulResponse<bool>> processBool(const RawClient::HttpResponse& response);
  folly::Optional<ConsulResponse<Value>> processValue(const RawClient::HttpResponse& response);
  folly::Optional<ConsulResponse<std::vector<Value>>> processValues(const RawClient::HttpResponse& response);
};

}  // namespace consul
}  // namespace service_framework
