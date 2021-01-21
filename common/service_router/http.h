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

#include "proxygen/httpserver/HTTPServerOptions.h"
#include "proxygen/httpserver/HTTPServer.h"

#include "router.h"
#include "common/http/http_server_manager.h"

namespace service_router {

using HttpServerModifier = folly::Optional<folly::Function<void(proxygen::HTTPServerOptions&)>>;

std::shared_ptr<proxygen::HTTPServer> createAndRegisterHttpServer(const ServerOption& option,
                                                       HttpServerModifier server_modifier = folly::none,
                                                       int wait_milliseconds = 0,
                                                       const ServerStatus& status = ServerStatus::AVAILABLE);

void httpServiceServer(const ServerOption& option, HttpServerModifier server_modifier = folly::none,
                       int wait_milliseconds = 0,
                       const ServerStatus& status = ServerStatus::AVAILABLE);
void httpServiceServer(const std::string& service_name, const ServerAddress& address,
                       HttpServerModifier server_modifier = folly::none, int wait_milliseconds = 0,
                       const ServerStatus& status = ServerStatus::AVAILABLE);
void httpServiceServer(const std::string& service_name, const std::string& host, uint16_t port,
                       HttpServerModifier server_modifier = folly::none, int wait_milliseconds = 0,
                       const ServerStatus& status = ServerStatus::AVAILABLE);
// anonymously service, no registration in consul
void httpServiceServer(const std::string& host, uint16_t port);

}  // namespace service_router
