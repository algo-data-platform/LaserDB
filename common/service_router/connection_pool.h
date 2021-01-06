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

#include <string>

#include "folly/synchronization/Baton.h"
#include "folly/io/async/EventBase.h"
#include "folly/Random.h"
#include "thrift/lib/cpp2/transport/core/ClientConnectionIf.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/transport/http2/client/H2ClientConnection.h"
#include "folly/io/async/AsyncSocket.h"
#include "folly/Singleton.h"
#include "city.h"

namespace service_router {

using Http2ClientConnectionIfPtr = std::shared_ptr<apache::thrift::ClientConnectionIf>;
using HeaderClientChannelPtr = std::shared_ptr<apache::thrift::HeaderClientChannel>;

template <typename TransportType>
class ConnectionThreadBase {
 public:
  explicit ConnectionThreadBase(folly::EventBase* evb) : eventbase_(evb) {}
  virtual ~ConnectionThreadBase() {
    VLOG(10) << "Delete connection thread";
    eventbase_->runInEventBaseThreadAndWait([this] { connections_.wlock()->clear(); });
  }
  TransportType getConnection(const std::string& addr, uint16_t port, uint64_t hash_key) {
    return connections_.withULockPtr([&addr, port, hash_key, this](auto ulock) {
      if (ulock->find(hash_key) != ulock->end() && ulock->at(hash_key)->good()) {
        return ulock->at(hash_key);
      }

      auto wlock = ulock.moveFromUpgradeToWrite();
      // 请勿使用 insert, insert 如果 key 相同不会覆盖原值，导致假如连接失活不会创建新连接的问题
      (*wlock)[hash_key] = std::move(createConnection(addr, port));
      return wlock->at(hash_key);
    });
  }

 protected:
  TransportType createConnection(const std::string& addr, uint16_t port) {
    TransportType conn;
    eventbase_->runInEventBaseThreadAndWait([&addr, port, &conn, this] {
      folly::AsyncSocket::UniquePtr socket = folly::AsyncSocket::newSocket(eventbase_, addr, port);
      socket->setZeroCopy(true);
      socket->setNoDelay(true);
      conn = createClientConnection(std::move(socket));
    });
    return conn;
  }

  virtual TransportType createClientConnection(folly::AsyncSocket::UniquePtr socket) = 0;

  folly::Synchronized<std::unordered_map<uint64_t, TransportType>> connections_;
  folly::EventBase* eventbase_;
};

template <typename TransportType>
class ConnectionThread : public ConnectionThreadBase<TransportType> {
 public:
  explicit ConnectionThread(folly::EventBase* evb) : ConnectionThreadBase<TransportType>(evb) {}
  virtual ~ConnectionThread() = default;

  // 不需要实现通用的 createClientConnection
};

// transport http2 的特化实现
template <>
class ConnectionThread<Http2ClientConnectionIfPtr> : public ConnectionThreadBase<Http2ClientConnectionIfPtr> {
 public:
  explicit ConnectionThread(folly::EventBase* evb) : ConnectionThreadBase<Http2ClientConnectionIfPtr>(evb) {}
  virtual ~ConnectionThread() = default;

 protected:
  Http2ClientConnectionIfPtr createClientConnection(folly::AsyncSocket::UniquePtr socket) {
    return apache::thrift::H2ClientConnection::newHTTP2Connection(std::move(socket));
  }
};

// 重写 HeaderClientChannel destroy 方法，使其转发到 evb 线程中回收
class SafeHeaderClientChannel : public apache::thrift::HeaderClientChannel {
 public:
  explicit SafeHeaderClientChannel(std::shared_ptr<folly::AsyncTransport> transport)
      : apache::thrift::HeaderClientChannel(transport) {}

  explicit SafeHeaderClientChannel(const std::shared_ptr<apache::thrift::Cpp2Channel>& cpp2Channel)
      : apache::thrift::HeaderClientChannel(cpp2Channel) {}

  static apache::thrift::HeaderClientChannel::Ptr newSafeChannel(
      const std::shared_ptr<folly::AsyncTransport>& transport) {
    return apache::thrift::HeaderClientChannel::Ptr(new SafeHeaderClientChannel(transport));
  }

  void destroy() override {
    getEventBase()->runInEventBaseThread([this]() { apache::thrift::HeaderClientChannel::destroy(); });
  }
};

// transport header 的特化实现
template <>
class ConnectionThread<HeaderClientChannelPtr> : public ConnectionThreadBase<HeaderClientChannelPtr> {
 public:
  explicit ConnectionThread(folly::EventBase* evb) : ConnectionThreadBase<HeaderClientChannelPtr>(evb) {}
  virtual ~ConnectionThread() = default;

 protected:
  HeaderClientChannelPtr createClientConnection(folly::AsyncSocket::UniquePtr socket) {
     return std::shared_ptr<apache::thrift::HeaderClientChannel>(
         SafeHeaderClientChannel::newSafeChannel(std::move(socket)));
  }
};

template <typename TransportType>
class ConnectionPool;
template <typename TransportType>
folly::Singleton<ConnectionPool<TransportType>> thrift_connection_pool =
    folly::Singleton<ConnectionPool<TransportType>>().shouldEagerInit();

//
// Thrift 客户端连接池
//
// 连接池试用全局 IO 线程池，每个线程一个 eventbase， 每个 eventbase 对应多个连接
//
// pool -> eventbase(one thread) -> connections
//
template <typename TransportType>
class ConnectionPool {
 public:
  static std::shared_ptr<ConnectionPool<TransportType>> getInstance() {
    return thrift_connection_pool<TransportType>.try_get();
  }
  ConnectionPool() {
    // folly::getEventBase() 通过轮询的方式返回各个线程对应的 eventbase， 所以每一圈返回的顺序是一致的
    // 为了确保正确我们多轮询一圈进行确认
    evb_list_.withWLock([&](auto& indexs) {
      int try_nums = 3;
      folly::EventBase* start = folly::getEventBase();
      while (try_nums > 0) {
        folly::EventBase* evb = folly::getEventBase();
        if (evb == start) {
          try_nums--;
        }
        if (std::find(indexs.begin(), indexs.end(), evb) != indexs.end()) {
          continue;
        }
        indexs.push_back(evb);
      }
    });
  }
  ~ConnectionPool() {
    VLOG(10) << "Delete connection pool";
    stop();
  }
  void stop() { threads_.wlock()->clear(); }

  TransportType getConnection(const std::string& addr, uint16_t port, int max_conn_per_server) {
    if (max_conn_per_server == 0) {
      max_conn_per_server = evb_list_.rlock()->size();
    }

    int expand_index = folly::Random::secureRand32(0, max_conn_per_server);
    uint64_t hash_key = CityHash64WithSeed(addr.c_str(), addr.size(), port + expand_index);
    int thread_id = hash_key % evb_list_.rlock()->size();
    folly::EventBase* evb = evb_list_.rlock()->at(thread_id);
    auto thread = threads_.withULockPtr([&](auto ulock) {
      if (ulock->find(evb) != ulock->end()) {
        return ulock->at(evb);
      }

      auto wlock = ulock.moveFromUpgradeToWrite();
      wlock->insert({evb, std::make_shared<ConnectionThread<TransportType>>(evb)});

      auto rlock = wlock.moveFromWriteToRead();
      return rlock->at(evb);
    });

    return thread->getConnection(addr, port, hash_key);
  }

 private:
  folly::Synchronized<std::unordered_map<folly::EventBase*, std::shared_ptr<ConnectionThread<TransportType>>>> threads_;
  folly::Synchronized<std::vector<folly::EventBase*>> evb_list_;
};

void stop_connection_pool();

}  // namespace service_router
