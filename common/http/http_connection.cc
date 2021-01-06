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

#include <string>

#include "folly/FileUtil.h"
#include "folly/String.h"
#include "folly/GLog.h"
#include "folly/io/async/SSLContext.h"
#include "folly/io/async/SSLOptions.h"
#include "folly/portability/GFlags.h"
#include "proxygen/lib/http/HTTPMessage.h"
#include "proxygen/lib/http/session/HTTPUpstreamSession.h"
#include "proxygen/lib/http/codec/HTTP2Codec.h"
#include "proxygen/lib/utils/WheelTimerInstance.h"

#include "http_client_manager.h"
#include "http_connection.h"

DEFINE_int32(http_recv_window, 65536, "Flow control receive window for h2/spdy");

namespace service_framework {
namespace http {

void HttpConnection::onDelayedDestroy(bool delayed) {
  if (!is_detached_) {
    return;
  }

  if (conn_->isBusy()) {
    conn_->reset();
  }
  http_manager_->detach(this);
  (void)delayed;
}

HttpConnection::HttpConnection(HttpClientManager* http_manager, uint64_t id, folly::EventBase* evb,
                               const proxygen::HTTPMessage& request, std::chrono::milliseconds connect_timeout,
                               std::chrono::milliseconds timeout)
    : http_manager_(http_manager),
      id_(id),
      evb_(evb),
      request_message_(request),
      timeout_(timeout),
      connect_timeout_(connect_timeout) {
  evb_->runInEventBaseThreadAndWait([this]() {
    proxygen::WheelTimerInstance timer(timeout_, evb_);
    timer_ = std::move(timer);
  });
  response_ = std::make_shared<HttpResponse>();
}

HttpConnection::HttpConnection(HttpClientManager* http_manager, uint64_t id, folly::EventBase* evb,
                               const proxygen::HTTPMessage& request, std::unique_ptr<folly::IOBuf> data,
                               std::chrono::milliseconds connect_timeout, std::chrono::milliseconds timeout)
    : HttpConnection(http_manager, id, evb, request, connect_timeout, timeout) {
  data_ = std::move(data);
}

folly::Future<std::shared_ptr<HttpResponse>> HttpConnection::call() {
  DestructorGuard g(this);
  folly::Future<std::shared_ptr<HttpResponse>> result = promise_.getFuture();
  conn_ = std::make_shared<proxygen::HTTPConnector>(this, timer_);

  const folly::SocketOptionMap opts{{{SOL_SOCKET, SO_REUSEADDR}, 1}};

  try {
    proxygen::URL url(request_message_.getURL());

    folly::SocketAddress addr = folly::SocketAddress(url.getHost(), url.getPort(), true);
    evb_->runInEventBaseThread([opts, addr, this]() {
      conn_->connect(evb_, addr, connect_timeout_, opts);
    });
  }
  catch (std::system_error& e) {  // 可能会由于域名解析错误抛异常
    LOG(ERROR) << "URL: " << request_message_.getURL() << " error:" << e.what();
    response_->setError(HttpResponseError::HTTP_DNS_ERROR);
    promise_.setValue(response_);
    is_detached_ = true;
  }
  return result;
}

uint64_t HttpConnection::getId() { return id_; }

void HttpConnection::connectSuccess(proxygen::HTTPUpstreamSession* session) {
  session->setFlowControl(FLAGS_http_recv_window, FLAGS_http_recv_window, FLAGS_http_recv_window);
  sendRequest(session->newTransaction(this));
  session->closeWhenIdle();
}

void HttpConnection::sendRequest(proxygen::HTTPTransaction* txn) {
  txn_ = txn;
  proxygen::URL url(request_message_.getURL());
  if (!request_message_.getHeaders().getNumberOfValues(proxygen::HTTP_HEADER_ACCEPT)) {
    request_message_.getHeaders().add("Accept", "*/*");
  }
  if (!request_message_.getHeaders().getNumberOfValues(proxygen::HTTP_HEADER_USER_AGENT)) {
    request_message_.getHeaders().add(proxygen::HTTP_HEADER_USER_AGENT, "proxygen_http");
  }
  if (!request_message_.getHeaders().getNumberOfValues(proxygen::HTTP_HEADER_HOST)) {
    request_message_.getHeaders().add(proxygen::HTTP_HEADER_HOST, url.getHostAndPort());
  }
  request_message_.setURL(url.makeRelativeURL());
  if (data_) {
    request_message_.getHeaders().add("Content-Length", folly::to<std::string>(data_->computeChainDataLength()));
  }
  txn_->sendHeaders(request_message_);

  folly::Optional<proxygen::HTTPMethod> method = request_message_.getMethod();
  if (method && (*method == proxygen::HTTPMethod::POST || *method == proxygen::HTTPMethod::PUT) && data_) {
    txn_->sendBody(std::move(data_));
  }

  VLOG(4) << "Send request: " << request_message_;
  txn_->sendEOM();
}

void HttpConnection::connectError(const folly::AsyncSocketException& ex) {
  DestructorGuard g(this);
  LOG(ERROR) << "couldn't connect to " << request_message_.getURL() << ":" << ex.what();
  response_->setError(HttpResponseError::HTTP_CONNECT_ERROR);
  promise_.setValue(response_);
  is_detached_ = true;
}

void HttpConnection::setTransaction(proxygen::HTTPTransaction*) noexcept {}

void HttpConnection::detachTransaction() noexcept {
  DestructorGuard g(this);
  promise_.setValue(response_);
  is_detached_ = true;
}

void HttpConnection::onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
  response_->setMessage(std::move(msg));
}

void HttpConnection::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
  if (chain) {
    response_->setBody(std::move(chain));
  }
}

void HttpConnection::onTrailers(std::unique_ptr<proxygen::HTTPHeaders>) noexcept {
  FB_LOG_EVERY_MS(INFO, 1000) << "Discarding trailers";
}

void HttpConnection::onEOM() noexcept { VLOG(10) << "Got EOM"; }

void HttpConnection::onUpgrade(proxygen::UpgradeProtocol) noexcept {
  FB_LOG_EVERY_MS(INFO, 1000) << "Discarding upgrade protocol";
}

void HttpConnection::onError(const proxygen::HTTPException& error) noexcept {
  DestructorGuard g(this);
  LOG(ERROR) << "An error occurred: " << error.what();
  response_->setError(HttpResponseError::HTTP_ERROR);
}

void HttpConnection::onEgressPaused() noexcept { FB_LOG_EVERY_MS(INFO, 1000) << "Egress paused"; }

void HttpConnection::onEgressResumed() noexcept { FB_LOG_EVERY_MS(INFO, 1000) << "Egress resumed"; }

}  // namespace http
}  // namespace service_framework
