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

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "folly/Synchronized.h"

#include "common/metrics/metrics.h"
#include "common/metrics/transform.h"
#include "common/util.h"
#include "proxygen/httpserver/RequestHandlerFactory.h"
#include "proxygen/httpserver/ResponseBuilder.h"
#include "proxygen/lib/http/HTTPMessage.h"

namespace service_framework {
namespace http {

class ServerResponse {
 public:
  explicit ServerResponse(proxygen::ResponseBuilder& builder) : response_builder_(builder) {}
  ~ServerResponse() = default;
  ServerResponse& status(uint16_t code, const std::string& message) {
    response_builder_.status(code, message);
    return *this;
  }

  template <typename T>
  ServerResponse& header(const std::string& header_in, const T& value) {
    response_builder_.header(header_in, value);
    return *this;
  }

  ServerResponse& body(std::unique_ptr<folly::IOBuf> body_in) {
    response_builder_.body(std::move(body_in));
    return *this;
  }

  template <typename T>
  ServerResponse& body(T&& t) {
    return body(folly::IOBuf::maybeCopyBuffer(folly::to<std::string>(std::forward<T>(t))));
  }

  void send() { response_builder_.send(); }

 private:
  proxygen::ResponseBuilder& response_builder_;
};

using LocationCallback =
    folly::Function<void(ServerResponse* /* response */, std::unique_ptr<proxygen::HTTPMessage> /* headers */,
                         std::unique_ptr<folly::IOBuf> /* body */)>;

class Location {
 public:
  Location(const std::string& location, LocationCallback callback, const proxygen::HTTPMethod& method)
      : location_(location), callback_(std::move(callback)), method_(method) {}
  ~Location() = default;

  void process(ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> headers,
               std::unique_ptr<folly::IOBuf> body) {
    callback_(response, std::move(headers), std::move(body));
  }

  const proxygen::HTTPMethod& getMethod() { return method_; }

 private:
  std::string location_;
  LocationCallback callback_;
  proxygen::HTTPMethod method_;
};

class HttpServerManager {
 public:
  static std::shared_ptr<HttpServerManager> getInstance();
  HttpServerManager() = default;
  ~HttpServerManager() = default;
  void registerLocation(const std::string& location, LocationCallback callbck,
                        const proxygen::HTTPMethod& method = proxygen::HTTPMethod::GET);
  void unregisterLocation(const std::string& location);
  folly::Optional<std::shared_ptr<Location>> getLocation(const std::string& location);

 private:
  folly::Synchronized<std::unordered_map<std::string, std::shared_ptr<Location>>> locations_;
};

class HttpHandler : public proxygen::RequestHandler {
 public:
  explicit HttpHandler(metrics::Timers* timers) : timers_(timers) {
    timer_ = std::make_unique<metrics::Timer>(timers_);
  }
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;

 private:
  metrics::Timers* timers_{nullptr};
  std::unique_ptr<metrics::Timer> timer_;
  std::unique_ptr<folly::IOBuf> body_;
  std::unique_ptr<proxygen::HTTPMessage> header_;
  bool is_valid_{false};
  void processErrorResponse(uint16_t code, const std::string& reason, const std::string& path);
};

class HttpHandlerFactory : public proxygen::RequestHandlerFactory {
 public:
  void onServerStart(folly::EventBase* /*evb*/) noexcept override;
  void onServerStop() noexcept override {}
  proxygen::RequestHandler* onRequest(proxygen::RequestHandler*, proxygen::HTTPMessage*) noexcept override {
    // 此处使用 new 操作，在 HttpHandler 内部会析构掉自身
    return new HttpHandler(timers_.get());
  }

 private:
  std::shared_ptr<metrics::Timers> timers_;
};

const std::vector<std::string> getModulesName(const std::string& module_pattern);
void initMetrics(const std::string& host, uint16_t port, const std::string& service_name);
void initGlogvs(const std::string& host, uint16_t port, const std::string& service_name);
void resultJson(ServerResponse* response, const folly::dynamic& data);
void errorJson(ServerResponse* response, uint32_t code, const std::string& error);
void sendResponse(ServerResponse* response, uint32_t code, const std::string& error, const folly::dynamic& data);

}  // namespace http
}  // namespace service_framework
