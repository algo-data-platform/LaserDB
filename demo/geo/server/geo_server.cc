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

#include "folly/init/Init.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "common/service_router/thrift.h"
#include "common/service_router/http.h"

#include "geo_service.h"

DEFINE_string(host, "127.0.0.1", "Geo Server port");
DEFINE_int32(http_port, 0, "Geo Server http port");
DEFINE_int32(port, 0, "Geo Server port");
DEFINE_string(service_name, "geo", "Geo Server port");
DEFINE_int32(load_balance_weight, 10, "Geo Server load balance weight");

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  SCOPE_EXIT {
    service_router::unregisterAll();
    service_framework::http::stop();
    service_router::stop_connection_pool();
  };

  std::shared_ptr<geo::GeoService> handler = std::make_shared<geo::GeoService>();

  std::thread http_server_thread([FLAGS_service_name, FLAGS_host, FLAGS_http_port]() {
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_http_port);
  });

  auto thrift_server_modifier = [](service_router::ThriftServer&) {};
  auto server_on_create = [FLAGS_load_balance_weight](const service_router::Server& server) {
    service_router::Router::getInstance()->setWeight(server, FLAGS_load_balance_weight);
  };
  service_router::thriftServiceServer<geo::GeoService>(FLAGS_service_name, FLAGS_host, FLAGS_port, handler,
                                                       thrift_server_modifier, 0,
                                                       service_router::ServerStatus::AVAILABLE, server_on_create);

  http_server_thread.join();

  return 0;
}
