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

#include "http_client_manager.h"

#include "folly/Singleton.h"
#include "folly/portability/GFlags.h"
#include "folly/executors/GlobalExecutor.h"

DEFINE_int32(http_client_default_connect_timeout, 100, "Http client 调用默认连接超时时间");
DEFINE_int32(http_client_default_socket_timeout, 500, "Http client 调用默认读写超时时间");

namespace service_framework {
namespace http {

folly::Singleton<HttpClientManager> globalHttpManager = folly::Singleton<HttpClientManager>().shouldEagerInit();

std::shared_ptr<HttpClientManager> HttpClientManager::getInstance() { return globalHttpManager.try_get(); }

folly::Future<std::shared_ptr<HttpResponse>> HttpClientManager::call(const proxygen::HTTPMessage& request,
                                                                     std::unique_ptr<folly::IOBuf> data,
                                                                     std::chrono::milliseconds connect_timeout,
                                                                     std::chrono::milliseconds timeout) {
  uint64_t id = genUniqueConnId();
  auto client = std::make_shared<HttpConnection>(this, id, folly::getEventBase(), request, std::move(data),
                                                 connect_timeout, timeout);
  connections_.wlock()->insert({id, client});
  return client->call();
}

folly::Future<std::shared_ptr<HttpResponse>> HttpClientManager::call(const proxygen::HTTPMessage& request,
                                                                     std::chrono::milliseconds connect_timeout,
                                                                     std::chrono::milliseconds timeout) {
  uint64_t id = genUniqueConnId();
  auto client = std::make_shared<HttpConnection>(this, id, folly::getEventBase(), request, connect_timeout, timeout);
  connections_.wlock()->insert({id, client});
  return client->call();
}

void HttpClientManager::detach(HttpConnection* client) {
  uint64_t id = client->getId();
  connections_.wlock()->erase(id);

  if (ready_stop_) {
    checkStop();
  }
}

uint64_t HttpClientManager::genUniqueConnId() {
  // result 可能不是最新的 conn_id ，但是可以保证发号唯一即可
  folly::SpinLockGuard g(spinlock_);
  uint64_t result = unique_conn_id_++;
  if (result >= UINT64_MAX - 10000) {
    unique_conn_id_.store(0);
    result = unique_conn_id_++;
  }

  return result;
}

void HttpClientManager::stop() {
  ready_stop_ = true;

  // 等待所有连接全部关闭
  checkStop();
  stop_.wait();
}

void HttpClientManager::checkStop() {
  // 由于使用了 baton 做同步，需要保证 wait/post 调用只有一次
  if (connections_.rlock()->empty()) {
    stop_.post();
  }
}

HttpClientManager::~HttpClientManager() {
  // 由于使用的 io 线程池，当 io 线程池析构时会清除调用所有的 timer 计时器，所以会应发 http 请求超时
  // 直接默认析构会引起没有执行完的 http 请求超时
  stop();
}

void stop() {
  auto manager = service_framework::http::HttpClientManager::getInstance();
  manager->stop();
}

HttpOption::HttpOption()
    : HttpOption(FLAGS_http_client_default_connect_timeout, FLAGS_http_client_default_socket_timeout) {}

HttpOption::HttpOption(int connect_timeout, int socket_timeout)
    : connect_timeout_(std::chrono::milliseconds(connect_timeout)),
      socket_timeout_(std::chrono::milliseconds(socket_timeout)) {}

folly::Future<std::shared_ptr<HttpResponse>> makeGetRequest(
    const std::string& url, const HttpOption& option, folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  auto manager = service_framework::http::HttpClientManager::getInstance();
  if (!manager) {
    auto res = std::make_shared<HttpResponse>();
    res->setError(HttpResponseError::HTTP_ERROR);
    return folly::makeFuture<std::shared_ptr<HttpResponse>>(std::move(res));
  }
  proxygen::HTTPMessage request;
  request.rawSetURL(url);
  request.setMethod(proxygen::HTTPMethod::GET);
  // 进一步扩展请求
  request_modifier(request);
  return manager->call(request, option.getConnectTimeout(), option.getSocketTimeout());
}

folly::Future<std::shared_ptr<HttpResponse>> makeGetRequest(const std::string& url, const HttpOption& option) {
  return makeGetRequest(url, option, [&](proxygen::HTTPMessage& /* req */) {});
}

folly::Future<std::shared_ptr<HttpResponse>> makeGetRequest(const std::string& url) {
  HttpOption option;
  return makeGetRequest(url, option);
}

folly::Future<std::shared_ptr<HttpResponse>> makeDeleteRequest(
    const std::string& url, const HttpOption& option, folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  auto manager = service_framework::http::HttpClientManager::getInstance();
  if (!manager) {
    auto res = std::make_shared<HttpResponse>();
    res->setError(HttpResponseError::HTTP_ERROR);
    return folly::makeFuture<std::shared_ptr<HttpResponse>>(std::move(res));
  }
  proxygen::HTTPMessage request;
  request.rawSetURL(url);
  request.setMethod(proxygen::HTTPMethod::DELETE);
  // 进一步扩展请求
  request_modifier(request);
  return manager->call(request, option.getConnectTimeout(), option.getSocketTimeout());
}

folly::Future<std::shared_ptr<HttpResponse>> makeDeleteRequest(const std::string& url, const HttpOption& option) {
  return makeDeleteRequest(url, option, [&](proxygen::HTTPMessage& /* req */) {});
}

folly::Future<std::shared_ptr<HttpResponse>> makeDeleteRequest(const std::string& url) {
  HttpOption option;
  return makeDeleteRequest(url, option);
}

// POST warp
folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(const std::string& url, const std::string& data) {
  return makePostRequest(url, std::move(folly::IOBuf::copyBuffer(data)));
}

folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(const std::string& url, const std::string& data,
                                                             const HttpOption& option) {
  return makePostRequest(url, std::move(folly::IOBuf::copyBuffer(data)), option);
}

folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(
    const std::string& url, const std::string& data, const HttpOption& option,
    folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  return makePostRequest(url, std::move(folly::IOBuf::copyBuffer(data)), option, std::move(request_modifier));
}

folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(const std::string& url,
                                                             std::unique_ptr<folly::IOBuf> data) {
  return makePostRequest(url, std::move(data));
}

folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(const std::string& url, std::unique_ptr<folly::IOBuf> data,
                                                             const HttpOption& option) {
  return makePostRequest(url, std::move(data), option, [&](proxygen::HTTPMessage& /* req */) {});
}

folly::Future<std::shared_ptr<HttpResponse>> makePostRequest(
    const std::string& url, std::unique_ptr<folly::IOBuf> data, const HttpOption& option,
    folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  auto manager = service_framework::http::HttpClientManager::getInstance();
  if (!manager) {
    auto res = std::make_shared<HttpResponse>();
    res->setError(HttpResponseError::HTTP_ERROR);
    return folly::makeFuture<std::shared_ptr<HttpResponse>>(std::move(res));
  }
  proxygen::HTTPMessage request;
  request.rawSetURL(url);
  request.setMethod(proxygen::HTTPMethod::POST);
  request_modifier(request);
  return manager->call(request, std::move(data), option.getConnectTimeout(), option.getSocketTimeout());
}

// PUT warp
folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(const std::string& url, const std::string& data) {
  return makePutRequest(url, std::move(folly::IOBuf::copyBuffer(data)));
}

folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(const std::string& url, const std::string& data,
                                                            const HttpOption& option) {
  return makePutRequest(url, std::move(folly::IOBuf::copyBuffer(data)), option);
}

folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(
    const std::string& url, const std::string& data, const HttpOption& option,
    folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  return makePutRequest(url, std::move(folly::IOBuf::copyBuffer(data)), option, std::move(request_modifier));
}

folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(const std::string& url,
                                                            std::unique_ptr<folly::IOBuf> data) {
  return makePutRequest(url, std::move(data));
}

folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(const std::string& url, std::unique_ptr<folly::IOBuf> data,
                                                            const HttpOption& option) {
  return makePutRequest(url, std::move(data), option, [&](proxygen::HTTPMessage& /* req */) {});
}

folly::Future<std::shared_ptr<HttpResponse>> makePutRequest(
    const std::string& url, std::unique_ptr<folly::IOBuf> data, const HttpOption& option,
    folly::Function<void(proxygen::HTTPMessage&)> request_modifier) {
  auto manager = service_framework::http::HttpClientManager::getInstance();
  if (!manager) {
    auto res = std::make_shared<HttpResponse>();
    res->setError(HttpResponseError::HTTP_ERROR);
    return folly::makeFuture<std::shared_ptr<HttpResponse>>(std::move(res));
  }
  proxygen::HTTPMessage request;
  request.rawSetURL(url);
  request.setMethod(proxygen::HTTPMethod::PUT);
  request_modifier(request);
  return manager->call(request, std::move(data), option.getConnectTimeout(), option.getSocketTimeout());
}

}  // namespace http
}  // namespace service_framework
