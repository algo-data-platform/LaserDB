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

#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "router.h"
#include "thrift/lib/cpp2/transport/core/ThriftProcessor.h"
#include "thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h"

namespace service_router {

class ThriftServer : public apache::thrift::ThriftServer {
 public:
  ThriftServer() {}
  ~ThriftServer() { stopServe(); }
  ThriftServer(const ThriftServer&) = delete;
  ThriftServer& operator=(const ThriftServer&) = delete;

  template<typename ServiceHandler>
  void init(const ServerOption& option, std::shared_ptr<ServiceHandler> handler);
  void startServe(bool is_block = false);
  void stopServe();
 private:
  std::unique_ptr<apache::thrift::HTTP2RoutingHandler> createHTTP2RoutingHandler();
 private:
  std::shared_ptr<std::thread> serve_thread_;
};

template <typename ServiceHandler>
void ThriftServer::init(const ServerOption& option, std::shared_ptr<ServiceHandler> handler) {
  auto proc_factory = std::make_shared<apache::thrift::ThriftServerAsyncProcessorFactory<ServiceHandler>>(handler);
  setCPUWorkerThreadName("thrift_cpu_worker");
  setIdleTimeout(std::chrono::milliseconds(FLAGS_idle_timeout));
  setAddress(option.getServerAddress().getHost(), option.getServerAddress().getPort());
  setProcessorFactory(proc_factory);
  // addRoutingHandler(std::make_unique<apache::thrift::RSRoutingHandler>());
  addRoutingHandler(createHTTP2RoutingHandler());
  setReusePort(option.getReusePort());
  return;
}

}  // namespace service_router
