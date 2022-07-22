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

#include "laser_proxy_service.h"

namespace laser {

void LaserProxyRedisHandler::read(wangle::HandlerAdapter<std::string>::Context* ctx, std::string msg) {
  auto protocol = getOrCreateRedisProtocol(ctx);
  protocol->process(ctx, msg);
}

std::shared_ptr<RedisProtocol> LaserProxyRedisHandler::getOrCreateRedisProtocol(
    wangle::HandlerAdapter<std::string>::Context* ctx) {
  return redis_protocol_parses_.withULockPtr([this, ctx](auto ulock) {
    if (ulock->find(ctx) != ulock->end()) {
      return ulock->at(ctx);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto protocol = std::make_shared<RedisProtocol>();
    auto redisCommandProcess = std::make_shared<RedisCommandProcess>(this->laser_client_, this->proxy_config_);
    protocol->setWriteCallback([this, ctx](auto& msg) { this->write(ctx, msg); });
    protocol->setErrorCallback([this, ctx]() { this->releaseConnection(ctx); });
    protocol->setLaserGetCallback([redisCommandProcess](std::string * res, const std::string & key)
                                      ->bool { return redisCommandProcess->get(res, key); });
    protocol->setLaserSetCallback([redisCommandProcess](const std::string & key, const std::string & value)
                                      ->bool { return redisCommandProcess->set(key, value); });
    protocol->setLaserSetexCallback([redisCommandProcess](const std::string & key, int64_t milliseconds,
                                                          const std::string & value)
                                        ->bool { return redisCommandProcess->setex(key, milliseconds, value); });
    protocol->setLaserAppendCallback([redisCommandProcess](uint32_t * length, const std::string & key,
                                                           const std::string & value)
                                         ->bool { return redisCommandProcess->append(length, key, value); });
    protocol->setLaserExistsCallback([redisCommandProcess](uint32_t * res, const std::vector<std::string> & keys)
                                         ->bool { return redisCommandProcess->exists(res, keys); });
    protocol->setLaserMGetCallback([redisCommandProcess](std::vector<std::string> * res,
                                                         const std::vector<std::string> & keys)
                                       ->bool { return redisCommandProcess->mget(res, keys); });
    protocol->setLaserMSetCallback([redisCommandProcess](std::vector<std::string> * res,
                                                         const std::map<std::string, std::string> & kvs)
                                       ->bool { return redisCommandProcess->mset(res, kvs); });
    protocol->setLaserConfigGetCallback([redisCommandProcess](std::string * res, const std::string & key)
                                            ->bool { return redisCommandProcess->configGet(res, key); });
    protocol->setLaserConfigSetCallback([redisCommandProcess](const std::string & key, const std::string & value)
                                            ->bool { return redisCommandProcess->configSet(key, value); });
    protocol->setLaserHGetCallback([redisCommandProcess](std::string * res, const std::string & key,
                                                         const std::string & field)
                                       ->bool { return redisCommandProcess->hget(res, key, field); });
    protocol->setLaserHSetCallback([redisCommandProcess](const std::string & key, const std::string & field,
                                                         const std::string & value)
                                       ->bool { return redisCommandProcess->hset(key, field, value); });
    protocol->setLaserHMGetCallback([redisCommandProcess](std::vector<std::string> * res, const std::string & key,
                                                          const std::vector<std::string> & fields)
                                        ->bool { return redisCommandProcess->hmget(res, key, fields); });
    protocol->setLaserHMSetCallback([redisCommandProcess](const std::string & key,
                                                          const std::map<std::string, std::string> & fvs)
                                        ->bool { return redisCommandProcess->hmset(key, fvs); });
    protocol->setLaserHGetAllCallback([redisCommandProcess](std::map<std::string, std::string> * res,
                                                            const std::string & key)
                                          ->bool { return redisCommandProcess->hgetall(res, key); });
    protocol->setLaserHKeysCallback([redisCommandProcess](std::vector<std::string> * res, const std::string & key)
                                        ->bool { return redisCommandProcess->hkeys(res, key); });
    protocol->setLaserHLenCallback([redisCommandProcess](uint32_t * res, const std::string & key)
                                       ->bool { return redisCommandProcess->hlen(res, key); });
    protocol->setLaserHExistsCallback([redisCommandProcess](const std::string & key, const std::string & field)
                                          ->bool { return redisCommandProcess->hexists(key, field); });
    protocol->setLaserHDelCallback([redisCommandProcess](const std::string & key, const std::string & field)
                                       ->bool { return redisCommandProcess->hdel(key, field); });
    protocol->setLaserDelCallback([redisCommandProcess](uint32_t * res, const std::vector<std::string> & keys)
                                      ->bool { return redisCommandProcess->del(res, keys); });
    protocol->setLaserExpireCallback([redisCommandProcess](const std::string & key, int64_t time)
                                         ->bool { return redisCommandProcess->expire(key, time); });
    protocol->setLaserExpireAtCallback([redisCommandProcess](const std::string & key, int64_t time_at)
                                           ->bool { return redisCommandProcess->expireat(key, time_at); });
    protocol->setLaserTtlCallback([redisCommandProcess](int64_t * res, const std::string & key)
                                      ->bool { return redisCommandProcess->ttl(res, key); });
    protocol->setLaserDecrCallback([redisCommandProcess](int64_t * res, const std::string & key)
                                       ->bool { return redisCommandProcess->decr(res, key); });
    protocol->setLaserIncrCallback([redisCommandProcess](int64_t * res, const std::string & key)
                                       ->bool { return redisCommandProcess->incr(res, key); });
    protocol->setLaserDecrByCallback([redisCommandProcess](int64_t * res, const std::string & key, int64_t step)
                                         ->bool { return redisCommandProcess->decrby(res, key, step); });
    protocol->setLaserIncrByCallback([redisCommandProcess](int64_t * res, const std::string & key, int64_t step)
                                         ->bool { return redisCommandProcess->incrby(res, key, step); });
    protocol->setLaserZAddCallback([redisCommandProcess](uint32_t * res, const std::string & key,
                                                         const std::unordered_map<std::string, double> & in_values)
                                       ->bool { return redisCommandProcess->zadd(res, key, in_values); });
    protocol->setLaserZRangeByScoreCallback([redisCommandProcess](
        std::vector<LaserFloatScoreMember> * res, const std::string & key, double min,
        double max)->bool { return redisCommandProcess->zrangebyscore(res, key, min, max); });
    protocol->setLaserZRemRangeByScoreCallback([redisCommandProcess](
        uint32_t * res, const std::string & key, double min,
        double max)->bool { return redisCommandProcess->zremrangebyscore(res, key, min, max); });

    (*wlock)[ctx] = protocol;
    return wlock->at(ctx);
  });
}

void LaserProxyRedisHandler::readEOF(wangle::HandlerAdapter<std::string>::Context* ctx) {
  VLOG(3) << "Connection closed by remote host";
  releaseConnection(ctx);
}

void LaserProxyRedisHandler::readException(wangle::HandlerAdapter<std::string>::Context* ctx,
                                           folly::exception_wrapper e) {
  VLOG(0) << "Remote error:" << folly::exceptionStr(e);
  releaseConnection(ctx);
}

void LaserProxyRedisHandler::releaseConnection(wangle::HandlerAdapter<std::string>::Context* ctx) {
  redis_protocol_parses_.withULockPtr([this, ctx](auto ulock) {
    if (ulock->find(ctx) != ulock->end()) {
      auto wlock = ulock.moveFromUpgradeToWrite();
      wlock->erase(ctx);
    }
  });

  close(ctx);
}

void LaserProxyRedisHandler::transportActive(wangle::HandlerAdapter<std::string>::Context* ctx) {
  auto sock = ctx->getTransport();
  folly::SocketAddress localAddress;
  sock->getLocalAddress(&localAddress);
  VLOG(10) << "The localAddress is:" << localAddress.describe();
}

LaserProxyRedisPipeline::Ptr LaserProxyRedisPipelineFactory::newPipeline(
    std::shared_ptr<folly::AsyncTransportWrapper> sock) {
  auto pipeline = LaserProxyRedisPipeline::create();
  pipeline->addBack(wangle::AsyncSocketHandler(sock));
  pipeline->addBack(wangle::LineBasedFrameDecoder());
  pipeline->addBack(wangle::StringCodec());
  pipeline->addBack(LaserProxyRedisHandler(laser_client_, proxy_config_));
  pipeline->finalize();

  return pipeline;
}
}  // namespace laser
