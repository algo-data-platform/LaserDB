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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author Junpeng Liu <liujunpeng555@gmail.com>
 */

#pragma once

#include <memory>
#include <unordered_map>

#include "folly/io/IOBufQueue.h"
#include "folly/Synchronized.h"
#include "folly/String.h"

#include "wangle/bootstrap/ServerBootstrap.h"
#include "wangle/channel/AsyncSocketHandler.h"
#include "wangle/codec/LineBasedFrameDecoder.h"
#include "wangle/codec/StringCodec.h"

#include "redis.h"
#include "redis_command_process.h"

namespace laser {

using LaserProxyRedisPipeline = wangle::Pipeline<folly::IOBufQueue&, std::string>;
using LaserProxyRedisMap =
    folly::Synchronized<std::map<wangle::HandlerAdapter<std::string>::Context*, std::shared_ptr<RedisProtocol>>>;

class LaserProxyRedisHandler : public wangle::HandlerAdapter<std::string> {
 public:
  explicit LaserProxyRedisHandler(std::shared_ptr<laser::LaserClient> laser_client,
                                  std::shared_ptr<laser::ProxyConfig> proxy_config) :
    laser_client_(laser_client), proxy_config_(proxy_config) {}
  void read(wangle::HandlerAdapter<std::string>::Context* ctx, std::string msg);
  void readEOF(wangle::HandlerAdapter<std::string>::Context* ctx);
  void readException(wangle::HandlerAdapter<std::string>::Context* ctx, folly::exception_wrapper e);
  void transportActive(wangle::HandlerAdapter<std::string>::Context* ctx);
  void releaseConnection(wangle::HandlerAdapter<std::string>::Context* ctx);
  std::shared_ptr<RedisProtocol> getOrCreateRedisProtocol(wangle::HandlerAdapter<std::string>::Context* ctx);

 private:
  LaserProxyRedisMap redis_protocol_parses_;
  std::shared_ptr<laser::LaserClient> laser_client_;
  std::shared_ptr<laser::ProxyConfig> proxy_config_;
};

class LaserProxyRedisPipelineFactory : public wangle::PipelineFactory<LaserProxyRedisPipeline> {
 public:
  explicit LaserProxyRedisPipelineFactory(std::shared_ptr<laser::LaserClient> laser_client,
                                          std::shared_ptr<laser::ProxyConfig> proxy_config) :
    laser_client_(laser_client), proxy_config_(proxy_config) {}
  LaserProxyRedisPipeline::Ptr newPipeline(std::shared_ptr<folly::AsyncTransportWrapper> sock) override;

 private:
  std::shared_ptr<laser::LaserClient> laser_client_;
  std::shared_ptr<laser::ProxyConfig> proxy_config_;
};
}  // namespace laser
