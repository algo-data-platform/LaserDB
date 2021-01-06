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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author Junpeng Liu <liujunpeng555@gmail.com>
 */

#include "folly/init/Init.h"
#include "folly/executors/IOThreadPoolExecutor.h"

#include "common/service_router/http.h"
#include "common/service_router/router.h"

#include "proxy_config.h"
#include "laser_proxy_service.h"

DEFINE_int32(laser_proxy_port, 6379, "laser proxy server port");
DEFINE_string(host, "127.0.0.1", "Local machine IP adress");
DEFINE_int32(port, 8399, "HTTP port");
DEFINE_string(service_name, "laser_proxy", "laser proxy service name");
DEFINE_string(target_service_name, "laser_test", "laser server service name");
DEFINE_int32(io_threads, 0, "Number of io perform thread to use. (Default: cup cores)");
DEFINE_int32(thrift_call_threads, 0, "Number of thrift call io perform thread to use. (Default: cup cores)");

int main(int argc, char** argv) {
  FLAGS_logtostderr = true;
  folly::Init init(&argc, &argv);

  if (FLAGS_io_threads == 0) {
    FLAGS_io_threads = std::thread::hardware_concurrency();
  }
  if (FLAGS_thrift_call_threads == 0) {
    FLAGS_thrift_call_threads = std::thread::hardware_concurrency();
  }
  auto io_pool = std::make_shared<folly::IOThreadPoolExecutor>(
      FLAGS_io_threads, std::make_shared<folly::NamedThreadFactory>("IOThreadPool"));
  auto thrift_io_pool = std::make_shared<folly::IOThreadPoolExecutor>(
      FLAGS_thrift_call_threads, std::make_shared<folly::NamedThreadFactory>("ThriftCall"));

  folly::setIOExecutor(thrift_io_pool);
  SCOPE_EXIT {
    service_router::unregisterAll();
    service_framework::http::stop();
    service_router::stop_connection_pool();
    io_pool->stop();
    thrift_io_pool->stop();
  };

  // Http service
  std::thread http_server_thread([]() {
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_port);
  });

  http_server_thread.detach();

  // Register the service
  auto router = service_router::Router::getInstance();
  router->waitUntilConfig(FLAGS_service_name, 0);
  service_router::Server reg_server;
  reg_server.setServiceName(FLAGS_service_name);
  reg_server.setHost(FLAGS_host);
  reg_server.setPort(FLAGS_laser_proxy_port);
  reg_server.setProtocol(service_router::ServerProtocol::REDIS);
  service_router::registerServer(reg_server, 0);
  router->setStatus(reg_server, service_router::ServerStatus::AVAILABLE);

  std::shared_ptr<laser::LaserClient> laser_client = std::make_shared<laser::LaserClient>(FLAGS_target_service_name);
  laser_client->init();

  auto proxy_config = std::make_shared<laser::ProxyConfig>(router);
  proxy_config->init(FLAGS_service_name);

  wangle::ServerBootstrap<laser::LaserProxyRedisPipeline> server;
  server.childPipeline(std::make_shared<laser::LaserProxyRedisPipelineFactory>(laser_client, proxy_config));
  server.group(io_pool);
  server.bind(FLAGS_laser_proxy_port);
  server.waitForStop();

  return 0;
}
