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

// @ref fbthrift/thrift/lib/cpp2/transport/core/ThriftClient.h
#pragma once

#include <stdint.h>

#include <memory>

#include "folly/io/IOBuf.h"
#include "folly/portability/GFlags.h"
#include "folly/io/async/EventBase.h"
#include "thrift/lib/cpp/protocol/TProtocolTypes.h"
#include "thrift/lib/cpp2/async/ClientChannel.h"

namespace service_router {

DECLARE_string(router_thrift_transport);

//
// 该类主要是对 HeaderClientChannel RSocketClientChannel 进行包装
//
// HeaderClientChannel/RSocketClientChannel 其实实现了 ClientChannel 的所有的功能，但是它们的限制是发起请求线程
// 必须和 eventbase 线程是同一个线程，这样会制约业务开发的灵活性，所以通过该类统一进行包装，当 sendRequest 时
// 通过 eventbase->runInEventBaseThread 来保证 HeaderClientChannel/RSocketClientChannel 里的 sendRequest, 从而
// 实现了多个线程发送请求的安全性
//
// 此处可以参考 http2 的实现，http2 实现采用最新的 apache::thrift::ThriftClient 类，该类也是参考
// apache::thrift::ThriftClient 的思路
class ThriftClient : public apache::thrift::ClientChannel {
 public:
  // Use "Ptr" instead of "unique_ptr<ThriftClient>".
  using Ptr = std::unique_ptr<ThriftClient, folly::DelayedDestruction::Destructor>;

  // Creates a ThriftClient object that uses "connection".  Callbacks
  // for asynchronous RPCs are run on "callbackEvb".  Callbacks for
  // synchronous RPCs are run on the event base of the connection.
  ThriftClient(const std::shared_ptr<apache::thrift::ClientChannel>& channel, folly::EventBase* callbackEvb);

  // Creates a ThriftClient object that uses "connection".  Callbacks
  // for all RPCs are run on the event base of the connection.
  explicit ThriftClient(const std::shared_ptr<apache::thrift::ClientChannel>& channel);

  ThriftClient(const ThriftClient&) = delete;
  ThriftClient& operator=(const ThriftClient&) = delete;

  // begin RequestChannel methods
  void sendRequestResponse(
      const apache::thrift::RpcOptions& rpcOptions,
      folly::StringPiece,
      apache::thrift::SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      apache::thrift::RequestClientCallback::Ptr cb) override;

  void sendRequestNoResponse(
      const apache::thrift::RpcOptions& rpcOptions,
      folly::StringPiece,
      apache::thrift::SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      apache::thrift::RequestClientCallback::Ptr cb) override;

  folly::EventBase* getEventBase() const override;

  uint16_t getProtocolId() override;

  void setCloseCallback(apache::thrift::CloseCallback* cb) override;

  // end RequestChannel methods

  // begin ClientChannel methods
  folly::AsyncTransport* getTransport() override;
  bool good() override;
  apache::thrift::ClientChannel::SaturationStatus getSaturationStatus() override;
  void attachEventBase(folly::EventBase* eventBase) override;
  void detachEventBase() override;
  bool isDetachable() override;
  uint32_t getTimeout() override;
  void setTimeout(uint32_t ms) override;
  void closeNow() override;
  CLIENT_TYPE getClientType() override;
  // end ClientChannel methods

 protected:
  std::shared_ptr<apache::thrift::ClientChannel> channel_;
  folly::EventBase* callbackEvb_;

  // Destructor is private because this class inherits from
  // folly:DelayedDestruction.
  virtual ~ThriftClient();
};

}  // namespace service_router
