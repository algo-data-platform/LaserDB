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

#include "http.h"

namespace service_router {

DEFINE_bool(http_server_metrics_export, true, "Start http metrics export");
DEFINE_bool(http_server_glogv_export, true, "Start http glogv {set/get/module get/module set/module add}");
DEFINE_int32(http_server_work_thread_num, 1, "http server work thread num");

constexpr uint16_t WAIT_HTTP_SERVER_SETUP_INTERVAL_SEC = 1;

namespace {

void registerServer(std::shared_ptr<proxygen::HTTPServer> http_server, const ServerOption& option,
                    int wait_milliseconds, const ServerStatus& status, const std::string& service_name) {
  // not anonymously service, register on consul
  if (!service_name.empty()) {
    for (;;) {
      auto addresses = http_server->addresses();
      if (!addresses.empty() && addresses[0].address.getPort() != 0) {
        LOG(INFO) << "Http Server running on port: " << addresses[0].address.getPort();
        Server reg_server;
        reg_server.setServiceName(service_name);
        reg_server.setHost(option.getServerAddress().getHost());
        reg_server.setPort(addresses[0].address.getPort());
        reg_server.setProtocol(ServerProtocol::HTTP);
        registerServer(reg_server, wait_milliseconds);
        auto router = Router::getInstance();
        router->setStatus(reg_server, status);
        if (FLAGS_http_server_metrics_export) {
          service_framework::http::initMetrics(option.getServerAddress().getHost(), addresses[0].address.getPort(),
                                               service_name);
        }
        if (FLAGS_http_server_glogv_export) {
          service_framework::http::initGlogvs(option.getServerAddress().getHost(), addresses[0].address.getPort(),
                                              service_name);
        }

        break;
      }
      // Since we are registering the server async, time efficiency is not that critical. Therefore sleep is acceptable
      std::this_thread::sleep_for(std::chrono::seconds(WAIT_HTTP_SERVER_SETUP_INTERVAL_SEC));
    }
  }
}

std::shared_ptr<proxygen::HTTPServer> createHttpServer(const ServerOption& option, HttpServerModifier server_modifier,
                                                       int wait_milliseconds, const ServerStatus& status) {
  std::vector<proxygen::HTTPServer::IPConfig> IPs = {
      {folly::SocketAddress(option.getServerAddress().getHost(), option.getServerAddress().getPort(), true),
       proxygen::HTTPServer::Protocol::HTTP}};

  proxygen::HTTPServerOptions options;
  //  这里会导致进程不退出。注释掉
  //  options.shutdownOn = {SIGINT, SIGTERM};
  options.threads = FLAGS_http_server_work_thread_num;
  options.enableContentCompression = false;
  options.handlerFactories =
      proxygen::RequestHandlerChain().addThen<service_framework::http::HttpHandlerFactory>().build();
  auto http_server = std::make_shared<proxygen::HTTPServer>(std::move(options));

  if (server_modifier) {
    (*server_modifier)(options);
  }

  const std::string& service_name = option.getServiceName();

  auto router = Router::getInstance();
  // not anonymously service, register on consul
  if (!service_name.empty()) {
    router->waitUntilConfig(service_name, wait_milliseconds);
  }

  // bind ip:port
  http_server->bind(IPs);
  return http_server;
}

}  // namespace

// TODO(changyu): refactor with httpServiceServer
std::shared_ptr<proxygen::HTTPServer> createAndRegisterHttpServer(const ServerOption& option,
                                                                  HttpServerModifier server_modifier,
                                                                  int wait_milliseconds, const ServerStatus& status) {
  auto http_server = createHttpServer(option, std::move(server_modifier), wait_milliseconds, status);
  const std::string& service_name = option.getServiceName();
  std::thread registerServerThread([http_server, &option, wait_milliseconds, &status, &service_name] {
    registerServer(http_server, option, wait_milliseconds, status, service_name);
  });
  registerServerThread.join();
  return http_server;
}

void httpServiceServer(const ServerOption& option, HttpServerModifier server_modifier, int wait_milliseconds,
                       const ServerStatus& status) {
  auto http_server = createAndRegisterHttpServer(option, std::move(server_modifier), wait_milliseconds, status);
  http_server->start();
}

void httpServiceServer(const std::string& service_name, const ServerAddress& address,
                       HttpServerModifier server_modifier, int wait_milliseconds, const ServerStatus& status) {
  ServerOption option;
  option.setServiceName(service_name);
  option.setServerAddress(address);
  httpServiceServer(option, std::move(server_modifier), wait_milliseconds, status);
}

void httpServiceServer(const std::string& service_name, const std::string& host, uint16_t port,
                       HttpServerModifier server_modifier, int wait_milliseconds, const ServerStatus& status) {
  ServerAddress address;
  address.setHost(host);
  address.setPort(port);
  httpServiceServer(service_name, address, std::move(server_modifier), wait_milliseconds, status);
}

// anonymously service, no registration in consul
void httpServiceServer(const std::string& host, uint16_t port) {
  ServerAddress address;
  address.setHost(host);
  address.setPort(port);

  ServerOption option;
  option.setServiceName("");
  option.setServerAddress(address);
  httpServiceServer(option);
}

}  // namespace service_router
