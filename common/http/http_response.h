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

#include "proxygen/lib/http/HTTPMessage.h"

namespace service_framework {
namespace http {

enum class HttpResponseError {
  HTTP_OK,
  HTTP_CONNECT_ERROR,
  HTTP_DNS_ERROR,
  HTTP_ERROR
};

class HttpResponse {
 public:
  using OptionalHttpMessage = folly::Optional<std::unique_ptr<proxygen::HTTPMessage>>;
  using OptionalBody = folly::Optional<std::unique_ptr<folly::IOBuf>>;

  HttpResponse() = default;
  ~HttpResponse() = default;

  void setMessage(std::unique_ptr<proxygen::HTTPMessage> message) { message_ = std::move(message); }

  void setBody(std::unique_ptr<folly::IOBuf> body) {
    const folly::IOBuf* p = body.get();
    do {
      body_.append(reinterpret_cast<const char*>(p->data()), p->length());
      p = p->next();
    } while (p != body.get());
  }

  void setError(const HttpResponseError& error) { error_ = error; }

  OptionalHttpMessage moveMessage() {
    if (message_ && isOk()) {
      return OptionalHttpMessage(std::move(message_));
    }
    return folly::none;
  }

  folly::Optional<folly::fbstring> moveToFbString() const {
    if (isOk()) {
      return body_;
    }

    return folly::none;
  }

  const HttpResponseError& getError() const { return error_; }

  bool isOk() const { return error_ == HttpResponseError::HTTP_OK; }

 private:
  HttpResponseError error_{HttpResponseError::HTTP_OK};
  folly::fbstring body_;
  std::unique_ptr<proxygen::HTTPMessage> message_;
};
}  // namespace http
}  // namespace service_framework
