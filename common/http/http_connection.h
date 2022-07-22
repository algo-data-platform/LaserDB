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

#include "folly/io/async/EventBase.h"
#include "folly/io/async/DelayedDestructionBase.h"
#include "folly/futures/Future.h"
#include "proxygen/lib/http/HTTPConnector.h"
#include "proxygen/lib/http/session/HTTPTransaction.h"
#include "proxygen/lib/utils/URL.h"

#include "http_response.h"

namespace service_framework {
namespace http {

class HttpClientManager;

//
// HttpConnection 类只要用来管理请求连接、回话，该类是非线程安全的，不能直接使用
// 需要在 one thread --- one eventloop 环境下使用
class HttpConnection : public proxygen::HTTPConnector::Callback,
                       public proxygen::HTTPTransactionHandler,
                       public folly::DelayedDestructionBase {
 public:
  HttpConnection(HttpClientManager* context, uint64_t id, folly::EventBase* evb, const proxygen::HTTPMessage& request,
                 std::chrono::milliseconds connect_timeout, std::chrono::milliseconds timeout);
  HttpConnection(HttpClientManager* context, uint64_t id, folly::EventBase* evb, const proxygen::HTTPMessage& request,
                 std::unique_ptr<folly::IOBuf> data, std::chrono::milliseconds connect_timeout,
                 std::chrono::milliseconds timeout);

  folly::Future<std::shared_ptr<HttpResponse>> call();

  uint64_t getId();

  // HTTPConnector methods
  void connectSuccess(proxygen::HTTPUpstreamSession* session) override;
  void connectError(const folly::AsyncSocketException& ex) override;

  // HTTPTransactionHandler methods
  void setTransaction(proxygen::HTTPTransaction* txn) noexcept override;
  void detachTransaction() noexcept override;
  void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override;
  void onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
  void onError(const proxygen::HTTPException& error) noexcept override;
  void onEgressPaused() noexcept override;
  void onEgressResumed() noexcept override;

  void sendRequest(proxygen::HTTPTransaction* txn);

  ~HttpConnection() = default;

 protected:
  // DelayedDestructionBase methods
  void onDelayedDestroy(bool delayed) override;

  HttpClientManager* http_manager_;
  uint64_t id_;
  folly::EventBase* evb_{nullptr};
  proxygen::HTTPMessage request_message_;
  std::chrono::milliseconds timeout_;
  std::chrono::milliseconds connect_timeout_;
  proxygen::WheelTimerInstance timer_;
  std::shared_ptr<proxygen::HTTPConnector> conn_;
  folly::Promise<std::shared_ptr<HttpResponse>> promise_;

  std::shared_ptr<HttpResponse> response_;
  std::unique_ptr<folly::IOBuf> data_;

  proxygen::HTTPTransaction* txn_{nullptr};
  bool is_detached_ = false;
};
}  // namespace http
}  // namespace service_framework
