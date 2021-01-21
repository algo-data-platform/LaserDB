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

#include <string>
#include <vector>

#include "folly/dynamic.h"

#include "common/util.h"
#include "raw_client.h"
#include "params.h"
#include "entity.h"

namespace service_framework {
namespace consul {

class AgentClient : protected RawClient {
 public:
  AgentClient(const std::string& host, int port) : RawClient(host, port) {}
  AgentClient(const std::string& host, int port, int connect_timeout, int socket_timeout)
      : RawClient(host, port, connect_timeout, socket_timeout) {}
  ~AgentClient() = default;

  // sync request
  // async request
  folly::Future<RawClient::HttpResponse> getMembers();

  // process responses methods
  folly::Optional<ConsulResponse<std::vector<Member>>> processMembers(const RawClient::HttpResponse& response);
};

}  // namespace consul
}  // namespace service_framework
