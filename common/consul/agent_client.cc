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

#include "agent_client.h"

namespace service_framework {
namespace consul {

static constexpr char CONSUL_AGENT_MENBERS[] = "v1/agent/members";

folly::Future<RawClient::HttpResponse> AgentClient::getMembers() {
  return service_framework::http::makeGetRequest(createUrl(CONSUL_AGENT_MENBERS), getHttpOption());
}

folly::Optional<ConsulResponse<std::vector<Member>>> AgentClient::processMembers(
    const RawClient::HttpResponse& response) {
  auto message = response->moveMessage();
  if (!message) {
    return folly::none;
  }

  auto http_message = std::move(*message);
  LOG(INFO) << *(http_message.get());
  if (http_message->getStatusCode() != 200) {
    return folly::none;
  }
  auto body = response->moveToFbString();
  folly::dynamic values;
  if (!common::fromJson(&values, *body) || !values.isArray()) {
    LOG(INFO) << *body;
    return folly::none;
  }

  std::vector<Member> members;
  for (size_t i = 0; i < values.size(); i++) {
    Member member;
    if (member.deserialize(values[i])) {
      members.push_back(member);
    }
  }
  ConsulResponse<std::vector<Member>> consul_response(std::move(http_message), members);
  return folly::Optional<ConsulResponse<std::vector<Member>>>(consul_response);
}

}  // namespace consul
}  // namespace service_framework
