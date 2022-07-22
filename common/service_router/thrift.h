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
 */

#pragma once

#include <string>

#include "proxygen/httpserver/HTTPServerOptions.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/transport/core/ThriftClient.h"
#include "thrift/lib/cpp2/transport/core/ThriftProcessor.h"
#include "thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h"

#include "common/metrics/metrics.h"
#include "common/service_router/connection_pool.h"
#include "common/service_router/router.h"
#include "common/service_router/thrift_client.h"
#include "common/service_router/thrift_server.h"

namespace service_router {

using ThriftServerModifier = folly::Function<void(service_router::ThriftServer&)>;
using ThriftServerOnCreate = folly::Function<void(const Server&) const>;
using ThriftServiceCallCallback = folly::Function<void(const ServerAddress&) const>;

constexpr uint16_t WAIT_SERVER_SETUP_INTERVAL_SEC = 1;
constexpr char THRIFT_SERVICE_CALL_MODULE_NAME[] = "thrift_service_client";

constexpr char THRIFT_SERVICE_CALL_METRIC_GET_CLIENT_SUCCESS[] = "get_client_succ";
constexpr char THRIFT_SERVICE_CALL_METRIC_GET_CLIENT_ERROR[] = "get_client_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_TRANSPORT_ERROR[] = "transport_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_TRANSPORT_TIMEOUT[] = "transport_timeout";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_APP_TIMEOUT[] = "app_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_APP_ERROR[] = "app_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_UNKNOWN_ERROR[] = "unknown_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_STD[] = "std_exception";
constexpr char THRIFT_SERVICE_CALL_METRIC_EX_FINAL_ERROR[] = "final_error";
constexpr char THRIFT_SERVICE_CALL_METRIC_CALL_SUCC[] = "call_succ";
constexpr char THRIFT_SERVICE_CALL_METRIC_CALL[] = "service_call";
constexpr double THRIFT_SERVICE_CALL_METRIC_CALL_BUCKET_SIZE = 1.0;
constexpr double THRIFT_SERVICE_CALL_METRIC_CALL_MIN = 0.0;
constexpr double THRIFT_SERVICE_CALL_METRIC_CALL_MAX = 1000.0;

static ThriftServiceCallCallback defaultServiceCallCallback() {
  auto func = [](const ServerAddress&) {};
  return std::move(func);
}

// thrift server/client warp
template <typename AsyncClient>
folly::Optional<std::unique_ptr<AsyncClient>> thriftServiceClient(ServerAddress* address, const ClientOption& option) {
  std::unordered_map<std::string, std::string> tags = {{"target_service_name", option.getServiceName()}};
  if (address == nullptr || !serviceClient(address, option, FLAGS_discover_wait_milliseconds)) {
    metrics::Metrics::getInstance()
        ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_GET_CLIENT_ERROR, tags)
        ->mark();
    return folly::none;
  }
  VLOG(4) << "Select node result is :" << *address;

  std::unique_ptr<AsyncClient> client;
  auto transport = stringToThriftTransport(FLAGS_router_thrift_transport);
  if (transport && *transport == ThriftTransport::HEADER) {
    auto conn_pool = ConnectionPool<HeaderClientChannelPtr>::getInstance();
    auto channel = conn_pool->getConnection(address->getHost(), address->getPort(), option.getMaxConnPerServer());
    if (option.getThriftCompressionMethod() != 0) {
      channel->getEventBase()->runInEventBaseThread([channel, compress_method = option.getThriftCompressionMethod()]() {
        channel->setTransform(compress_method);
      });
    }
    auto thrift_client = service_router::ThriftClient::Ptr(new ThriftClient(channel));
    thrift_client->setTimeout(option.getTimeoutMs());
    client = std::make_unique<AsyncClient>(std::move(thrift_client));
  } else {
    auto conn_pool = ConnectionPool<Http2ClientConnectionIfPtr>::getInstance();
    auto client_conn = conn_pool->getConnection(address->getHost(), address->getPort(), option.getMaxConnPerServer());
    auto thrift_client =
        apache::thrift::ThriftClient::Ptr(new apache::thrift::ThriftClient(client_conn, client_conn->getEventBase()));
    thrift_client->setTimeout(option.getTimeoutMs());
    client = std::make_unique<AsyncClient>(std::move(thrift_client));
  }

  metrics::Metrics::getInstance()
      ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_GET_CLIENT_SUCCESS, tags)
      ->mark();
  return std::move(client);
}

template <typename AsyncClient>
folly::Optional<std::unique_ptr<AsyncClient>> thriftServiceClient(const std::string& service_name) {
  ClientOption option;
  option.setServiceName(service_name);
  option.setProtocol(ServerProtocol::THRIFT);
  ServerAddress address;
  return thriftServiceClient<AsyncClient>(&address, option);
}

static ThriftServerModifier defaultModifier() {
  auto func = [](service_router::ThriftServer&) {};
  return std::move(func);
}

template <typename AsyncClient>
bool thriftServiceCall(const ClientOption& option, folly::Function<void(std::unique_ptr<AsyncClient>)> request_process,
                       const folly::Optional<ThriftRetryOption>& retry_option = folly::none,
                       ThriftServiceCallCallback callback = defaultServiceCallCallback()) {
  std::unordered_map<std::string, std::string> tags = {{"target_service_name", option.getServiceName()}};
  ThriftRetryOption try_option;
  if (!retry_option) {
    try_option.setConnectionRetry(FLAGS_thrift_connection_retry);
    try_option.setTimeoutRetry(FLAGS_thrift_timeout_retry);
  } else {
    try_option = *retry_option;
  }

  int connection_retry = -1;
  int timeout_retry = -1;
  ServerAddress address;
  SCOPE_EXIT { callback(address); };
  while (connection_retry < try_option.getConnectionRetry() && timeout_retry < try_option.getTimeoutRetry()) {
    auto client = thriftServiceClient<AsyncClient>(&address, option);
    if (!client) {
      connection_retry++;
      if (connection_retry == try_option.getConnectionRetry()) {
        FB_LOG_EVERY_MS(ERROR, 2000) << "Get thrift client fail, service name:" << option.getServiceName()
                                     << " retry times: " << connection_retry;
      }
      continue;
    }

    try {
      auto timers = metrics::Metrics::getInstance()->buildTimers(
          THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_CALL, THRIFT_SERVICE_CALL_METRIC_CALL_BUCKET_SIZE,
          THRIFT_SERVICE_CALL_METRIC_CALL_MIN, THRIFT_SERVICE_CALL_METRIC_CALL_MAX, tags);
      metrics::Timer timer(timers.get());
      // real call
      request_process(std::move(*client));
      metrics::Metrics::getInstance()
          ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_CALL_SUCC, tags)
          ->mark();
      return true;
    } catch (apache::thrift::transport::TTransportException& ex) {
      if (ex.getType() == apache::thrift::transport::TTransportException::TTransportExceptionType::TIMED_OUT) {
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_TRANSPORT_TIMEOUT, tags)
            ->mark();
        timeout_retry++;
      } else {
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_TRANSPORT_ERROR, tags)
            ->mark();
        connection_retry++;
      }
      if (timeout_retry == try_option.getTimeoutRetry() || connection_retry == try_option.getConnectionRetry()) {
        FB_LOG_EVERY_MS(ERROR, 2000) << " service name:" << option.getServiceName() << " address:" << address
                                     << " retry times: "
                                     << ((connection_retry == try_option.getConnectionRetry()) ? connection_retry
                                                                                               : timeout_retry)
                                     << " ex:" << ex.what();
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_FINAL_ERROR, tags)
            ->mark();
      }
    } catch (apache::thrift::TApplicationException& ex) {
      if (ex.getType() == apache::thrift::TApplicationException::TApplicationExceptionType::TIMEOUT) {
        timeout_retry++;
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_APP_TIMEOUT, tags)
            ->mark();
      } else {
        connection_retry++;
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_APP_ERROR, tags)
            ->mark();
      }
      if (timeout_retry == try_option.getTimeoutRetry() || connection_retry == try_option.getConnectionRetry()) {
        FB_LOG_EVERY_MS(ERROR, 2000) << " service name:" << option.getServiceName() << " address:" << address
                                     << " retry times: "
                                     << ((connection_retry == try_option.getConnectionRetry()) ? connection_retry
                                                                                               : timeout_retry)
                                     << " ex:" << ex.what();
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_FINAL_ERROR, tags)
            ->mark();
      }
    } catch (std::exception& e) {
      FB_LOG_EVERY_MS(ERROR, 2000) << "address: " << address << "got unknown exception: " << e.what();
      metrics::Metrics::getInstance()
          ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_STD, tags)
          ->mark();
    } catch (...) {
      metrics::Metrics::getInstance()
          ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_UNKNOWN_ERROR, tags)
          ->mark();
      connection_retry++;
      if (connection_retry == try_option.getConnectionRetry()) {
        FB_LOG_EVERY_MS(ERROR, 2000) << "address:" << address << "has request fail.";
        metrics::Metrics::getInstance()
            ->buildMeter(THRIFT_SERVICE_CALL_MODULE_NAME, THRIFT_SERVICE_CALL_METRIC_EX_FINAL_ERROR, tags)
            ->mark();
      }
    }
  }
  return false;
}

template <typename AsyncClient>
bool thriftServiceCall(const std::string& service_name,
                       folly::Function<void(folly::Optional<std::unique_ptr<AsyncClient>>)> request_process) {
  ClientOption option;
  option.setServiceName(service_name);
  option.setProtocol(ServerProtocol::THRIFT);
  return thriftServiceCall<AsyncClient>(option, std::move(request_process));
}

static ThriftServerOnCreate defaultCreateCallback() {
  auto func = [](const Server&) {};
  return std::move(func);
}

static std::unique_ptr<apache::thrift::HTTP2RoutingHandler> createHTTP2RoutingHandler(
    std::shared_ptr<apache::thrift::ThriftServer> server) {
  auto h2_options = std::make_unique<proxygen::HTTPServerOptions>();
  h2_options->threads = static_cast<size_t>(server->getNumIOWorkerThreads());
  h2_options->idleTimeout = server->getIdleTimeout();
  h2_options->shutdownOn = {SIGINT, SIGTERM};
  return std::make_unique<apache::thrift::HTTP2RoutingHandler>(std::move(h2_options), server->getThriftProcessor(),
                                                               *server);
}

//
// thriftServiceServer 本身是 blocking 的，如果需要修改 server 属性可以通过 ThriftServerModifier 来修改
template <typename ServiceHandler>
void thriftServiceServer(const ServerOption& option, std::shared_ptr<ServiceHandler> handler,
                         ThriftServerModifier server_modifier = defaultModifier(), int wait_milliseconds = 0,
                         const ServerStatus& status = ServerStatus::AVAILABLE,
                         ThriftServerOnCreate on_create = defaultCreateCallback()) {
  auto thrift_server = std::make_shared<service_router::ThriftServer>();
  thrift_server->init<ServiceHandler>(option, handler);
  server_modifier(*thrift_server);

  // 先等待获取配置
  auto router = Router::getInstance();
  router->waitUntilConfig(option.getServiceName(), wait_milliseconds);

  // 可以通过设置 modifier 在 server 启动前修改属性

  Server server;
  // 由于要获取 thrift 最终 bind 操作后的端口，所以需要异步的来获取 port
  std::thread registerServerThread(
      [thrift_server, option, wait_milliseconds, router, status, &server, on_create_func = std::move(on_create)] {
        for (;;) {
          auto address = thrift_server->getAddress();
          if (address.getPort() != 0) {
            LOG(INFO) << "Thrift server running on port: " << address.getPort();
            server.setServiceName(option.getServiceName());
            server.setHost(option.getServerAddress().getHost());
            server.setPort(address.getPort());
            server.setProtocol(ServerProtocol::THRIFT);
            server.setStatus(status);
            registerServer(server, wait_milliseconds);
            on_create_func(server);
            break;
          }
          // 由于是异步注册 server 所以对时效性没有那么强，简单 sleep 可以接受
          std::this_thread::sleep_for(std::chrono::seconds(WAIT_SERVER_SETUP_INTERVAL_SEC));
        }
      });

  thrift_server->startServe(true);
  registerServerThread.join();
}

template <typename ServiceHandler>
void thriftServiceServer(const std::string& service_name, const ServerAddress& address,
                         std::shared_ptr<ServiceHandler> handler,
                         ThriftServerModifier server_modifier = defaultModifier(), int wait_milliseconds = 0,
                         const ServerStatus& status = ServerStatus::AVAILABLE,
                         ThriftServerOnCreate on_create = defaultCreateCallback()) {
  ServerOption option;
  option.setServiceName(service_name);
  option.setServerAddress(address);
  thriftServiceServer<ServiceHandler>(option, handler, std::move(server_modifier), wait_milliseconds, status,
                                      std::move(on_create));
}

template <typename ServiceHandler>
void thriftServiceServer(const std::string& service_name, const std::string& host, uint16_t port,
                         std::shared_ptr<ServiceHandler> handler,
                         ThriftServerModifier server_modifier = defaultModifier(), int wait_milliseconds = 0,
                         const ServerStatus& status = ServerStatus::AVAILABLE,
                         ThriftServerOnCreate on_create = defaultCreateCallback()) {
  ServerAddress address;
  address.setHost(host);
  address.setPort(port);
  return thriftServiceServer<ServiceHandler>(service_name, address, handler, std::move(server_modifier),
                                             wait_milliseconds, status, std::move(on_create));
}
}  // namespace service_router
