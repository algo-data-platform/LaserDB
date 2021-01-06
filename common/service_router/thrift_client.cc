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

// @ref fbthrift/thrift/lib/cpp2/transport/core/ThriftClient.cpp
#include "thrift_client.h"

#include "glog/logging.h"
#include "folly/io/async/Request.h"
#include "thrift/lib/cpp2/async/ResponseChannel.h"
#include "thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h"

namespace service_router {

DEFINE_string(router_thrift_transport, "header", "Transport to use: header, rsocket, http2");

ThriftClient::ThriftClient(const std::shared_ptr<apache::thrift::ClientChannel>& channel, folly::EventBase* callbackEvb)
    : channel_(channel), callbackEvb_(callbackEvb) {}

ThriftClient::ThriftClient(const std::shared_ptr<apache::thrift::ClientChannel>& channel)
    : ThriftClient(channel, channel->getEventBase()) {}

ThriftClient::~ThriftClient() { setCloseCallback(nullptr); }

void ThriftClient::sendRequestResponse(
      const apache::thrift::RpcOptions& rpcOptions,
      folly::StringPiece sp,
      apache::thrift::SerializedRequest&& sr,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      apache::thrift::RequestClientCallback::Ptr cb) {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([
    channel = std::move(channel),
    rpcOptions,
    sp = std::move(sp),
    sr = std::move(sr),
    cb = std::move(cb),
    header = std::move(header)
  ]() mutable { channel->sendRequestResponse(rpcOptions, std::move(sp), std::move(sr), std::move(header), std::move(cb)); });
}

void ThriftClient::sendRequestNoResponse(
      const apache::thrift::RpcOptions& rpcOptions,
      folly::StringPiece sp,
      apache::thrift::SerializedRequest&& sr,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      apache::thrift::RequestClientCallback::Ptr cb) {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([
    channel = std::move(channel),
    rpcOptions,
    sp = std::move(sp),
    sr = std::move(sr),
    cb = std::move(cb),
    header = std::move(header)
  ]() mutable { channel->sendRequestResponse(rpcOptions, std::move(sp), std::move(sr), std::move(header), std::move(cb)); });
}

folly::EventBase* ThriftClient::getEventBase() const { return channel_->getEventBase(); }

uint16_t ThriftClient::getProtocolId() { return channel_->getProtocolId(); }

void ThriftClient::setCloseCallback(apache::thrift::CloseCallback* cb) {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([
    channel = std::move(channel),
    cb
  ]() { channel->setCloseCallback(cb); });
}

folly::AsyncTransport* ThriftClient::getTransport() { return channel_->getTransport(); }

bool ThriftClient::good() { return channel_->good(); }

apache::thrift::ClientChannel::SaturationStatus ThriftClient::getSaturationStatus() {
  return channel_->getSaturationStatus();
}

void ThriftClient::attachEventBase(folly::EventBase* eventBase) {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([
    channel = std::move(channel),
    eventBase
  ]() { channel->attachEventBase(eventBase); });
}

void ThriftClient::detachEventBase() {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([channel = std::move(channel)]() { channel->detachEventBase(); });
}

bool ThriftClient::isDetachable() { return channel_->isDetachable(); }

uint32_t ThriftClient::getTimeout() { return channel_->getTimeout(); }

void ThriftClient::setTimeout(uint32_t ms) {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([
    channel = std::move(channel),
    ms
  ]() { channel->setTimeout(ms); });
}

void ThriftClient::closeNow() {
  auto channel = channel_;
  channel_->getEventBase()->runInEventBaseThread([channel = std::move(channel)]() { channel->closeNow(); });
}

CLIENT_TYPE ThriftClient::getClientType() { return channel_->getClientType(); }

}  // namespace service_router
