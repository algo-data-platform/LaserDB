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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author liubang <it.liubang@gmail.com>
 */

#include "laser_client.h"

#include "common/service_router/thrift.h"
#include "common/laser/if/gen-cpp2/LaserService.h"
#include "common/laser/config_manager.h"
#include "common/laser/partition.h"
#include "common/laser/status.h"

namespace laser {

DEFINE_int32(laser_client_thread_nums, 0, "Laser client work thread pool numbers");
DEFINE_int32(laser_client_max_conn_per_server, 0, "Laser client max connection per server");

constexpr char LASER_CLIENT_MODULE_NAME[] = "laser_client";
constexpr double LASER_CLIENT_METRIC_CALL_BUCKET_SIZE = 1.0;
constexpr double LASER_CLIENT_METRIC_CALL_MIN = 0.0;
constexpr double LASER_CLIENT_METRIC_CALL_MAX = 1000.0;
constexpr char FUTURE_TIMEOUT_EXCEPTION_MESSAGE[] = "future timeout";

constexpr char LASER_CLIENT_METRIC_CALL_SERVER_TIMES[] = "call_server_times";
constexpr char LASER_CLIENT_METRIC_CALL_SERVER_ERROR[] = "call_error";
constexpr char LASER_CLIENT_METRIC_COMMAND_DEL_TIMER[] = "del_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_EXPIRE_TIMER[] = "expire_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_EXPIREAT_TIMER[] = "expireat_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_TTL_TIMER[] = "ttl_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_GET_TIMER[] = "get_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_SET_TIMER[] = "set_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_APPEND_TIMER[] = "append_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_SETX_TIMER[] = "setx_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_MSET_TIMER[] = "mset_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_MSET_DETAIL_TIMER[] = "mset_detail_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_MDEL_TIMER[] = "mdel_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_ZADD_TIMER[] = "zadd_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_ZRANGEBYSCORE_TIMER[] = "zrangebyscore_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_ZREMRANGEBYSCORE_TIMER[] = "zremrangebyscore_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_MGET_TIMER[] = "mget_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_MGET_DETAIL_TIMER[] = "mget_detail_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_EXIST_TIMER[] = "exist_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HSET_TIMER[] = "hset_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HGET_TIMER[] = "hget_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HDEL_TIMER[] = "hdel_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HEXISTS_TIMER[] = "hexists_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HGETALL_TIMER[] = "hgetall_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HKEYS_TIMER[] = "hkeys_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HLEN_TIMER[] = "hlen_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HMSET_TIMER[] = "hmset_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_HMGET_TIMER[] = "hmget_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_LLEN_TIMER[] = "llen_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_LPOP_TIMER[] = "lpop_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_LPUSH_TIMER[] = "lpush_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_RPOP_TIMER[] = "rpop_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_RPUSH_TIMER[] = "rpush_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_LRANGE_TIMER[] = "lrange_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_LINDEX_TIMER[] = "lindex_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_DECR_TIMER[] = "decr_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_INCR_TIMER[] = "incr_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_DECRBY_TIMER[] = "decrby_command";
constexpr char LASER_CLIENT_METRIC_COMMAND_INCRBY_TIMER[] = "incrby_command";

ServerRelationKeys::ServerRelationKeys(std::shared_ptr<service_router::ServerAddress> address)
    : address_ptr_(address) {}

void ServerRelationKeys::addIndex(unsigned int index) { indexes_.push_back(index); }

folly::Singleton<LaserClientResource> global_laser_client_resource =
    folly::Singleton<LaserClientResource>().shouldEagerInit();

std::shared_ptr<LaserClientResource> LaserClientResource::getInstance() {
  return global_laser_client_resource.try_get();
}

LaserClientResource::LaserClientResource() {
  uint32_t thread_nums =
      (FLAGS_laser_client_thread_nums == 0) ? std::thread::hardware_concurrency() : FLAGS_laser_client_thread_nums;
  work_thread_pool_ = std::make_shared<folly::CPUThreadPoolExecutor>(
      thread_nums, std::make_shared<folly::NamedThreadFactory>("LaserClientWorkPool"));
}

LaserClientResource::~LaserClientResource() {
  if (work_thread_pool_) {
    work_thread_pool_->stop();
  }
}

std::shared_ptr<laser::ConfigManager> LaserClientResource::getOrCreateConfigManager(const std::string& service_name) {
  auto config = config_managers_.withULockPtr([&service_name](auto ulock) {
    if (ulock->find(service_name) != ulock->end()) {
      return ulock->at(service_name);
    }

    auto wlock = ulock.moveFromUpgradeToWrite();
    auto config_manager = std::make_shared<laser::ConfigManager>(service_router::Router::getInstance());
    config_manager->init(service_name);
    (*wlock)[service_name] = config_manager;
    return wlock->at(service_name);
  });
  return config;
}

void LaserClient::init() {
  config_manager_ = LaserClientResource::getInstance()->getOrCreateConfigManager(target_service_name_);
  work_thread_pool_ = LaserClientResource::getInstance()->getWorkPool();
  call_server_timers_ =
      metrics::Metrics::getInstance()->buildMeter(LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_CALL_SERVER_TIMES);

  Status status = Status::OK;
  std::unordered_map<std::string, std::string> tags = {{"error", statusToName(status)}};
  call_server_status_ok_ = metrics::Metrics::getInstance()->buildMeter(LASER_CLIENT_MODULE_NAME,
                                                                       LASER_CLIENT_METRIC_CALL_SERVER_ERROR, tags);
  del_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_DEL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  expire_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_EXPIRE_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  expireat_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_EXPIREAT_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  ttl_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_TTL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  get_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_GET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  set_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_SET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  append_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_APPEND_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  setx_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_SETX_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  mget_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_MGET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  mget_detail_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_MGET_DETAIL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  mset_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_MSET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  mset_detail_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_MSET_DETAIL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  mdel_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_MDEL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  zadd_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_ZADD_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  zrangebyscore_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_ZRANGEBYSCORE_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  zremrangebyscore_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_ZREMRANGEBYSCORE_TIMER,
      LASER_CLIENT_METRIC_CALL_BUCKET_SIZE, LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  exist_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_EXIST_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hset_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HSET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hget_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HGET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hdel_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HDEL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hexists_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HEXISTS_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hgetall_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HGETALL_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hkeys_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HKEYS_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hlen_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HLEN_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hmset_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HMSET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  hmget_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_HMGET_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  lindex_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_LINDEX_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  llen_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_LLEN_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  lpop_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_LPOP_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  lpush_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_LPUSH_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  lrange_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_LRANGE_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  rpop_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_RPOP_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  rpush_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_RPUSH_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  decr_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_DECR_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  incr_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_INCR_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  decrby_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_DECRBY_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
  incrby_command_timers_ = metrics::Metrics::getInstance()->buildTimers(
      LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_COMMAND_INCRBY_TIMER, LASER_CLIENT_METRIC_CALL_BUCKET_SIZE,
      LASER_CLIENT_METRIC_CALL_MIN, LASER_CLIENT_METRIC_CALL_MAX);
}

void LaserClient::getRetryOption(service_router::ThriftRetryOption* retry_option, const ClientOption& options) {
  retry_option->setConnectionRetry(options.getConnectionRetry());
  retry_option->setTimeoutRetry(options.getTimeoutRetry());
}

bool LaserClient::commonCall(const LaserKey& laser_key, const ClientOption& options,
                             ThriftProcessRequestFunc callback) {
  call_server_timers_->mark();
  uint32_t shard_id = UINT32_MAX;
  int64_t partition_hash = 0;
  bool route_to_edge_node = false;
  bool result = getRouteInfo(&shard_id, &partition_hash, &route_to_edge_node, laser_key, options);
  if (!result) {
    VLOG(5) << "Get route info fail, database_name:" << laser_key.get_database_name()
            << " table_name:" << laser_key.get_table_name();
    return false;
  }
  return callThriftServer(shard_id, partition_hash, route_to_edge_node, options, std::move(callback));
}

bool LaserClient::callThriftServer(uint32_t shard_id, int64_t partition_hash, bool route_to_edge_node,
                                   const ClientOption& options, ThriftProcessRequestFunc callback) {
  auto router_options = getClientOption(options, shard_id, partition_hash, route_to_edge_node);
  apache::thrift::RpcOptions rpc_options;
  service_router::ThriftRetryOption retry_option;
  rpc_options.setTimeout(std::chrono::milliseconds(options.getReceiveTimeoutMs()));
  LaserClient::getRetryOption(&retry_option, options);

  bool ret = service_router::thriftServiceCall<laser::LaserServiceAsyncClient>(
      *router_options,
      [ callback = std::move(callback), &rpc_options ](std::unique_ptr<laser::LaserServiceAsyncClient> client) {
                                                        callback(std::move(client), rpc_options);
                                                      },
      retry_option);
  if (!ret) {
    thread_local static std::unordered_map<std::string, std::string> tags = {{"error", "get_client_error"}};
    metrics::Metrics::getInstance()
        ->buildMeter(LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_CALL_SERVER_ERROR, tags)
        ->mark();
  }
  return ret;
}

bool LaserClient::callThriftServer(std::shared_ptr<service_router::ServerAddress> address, const ClientOption& options,
                                   ThriftProcessRequestFunc callback) {
  auto router_options = getClientOption(options);
  router_options->setTargetServerAddress(*address);
  apache::thrift::RpcOptions rpc_options;
  service_router::ThriftRetryOption retry_option;
  rpc_options.setTimeout(std::chrono::milliseconds(options.getReceiveTimeoutMs()));
  LaserClient::getRetryOption(&retry_option, options);

  bool ret = service_router::thriftServiceCall<laser::LaserServiceAsyncClient>(
      *router_options,
      [ callback = std::move(callback), &rpc_options ](std::unique_ptr<laser::LaserServiceAsyncClient> client) {
                                                        callback(std::move(client), rpc_options);
                                                      },
      retry_option);
  if (!ret) {
    thread_local static std::unordered_map<std::string, std::string> tags = {{"error", "get_client_error"}};
    metrics::Metrics::getInstance()
        ->buildMeter(LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_CALL_SERVER_ERROR, tags)
        ->mark();
  }
  return ret;
}

Status LaserClient::processSync(ThriftSendRequestFunc send_request, ThriftTryResponseProcessFunc process_func,
                                uint32_t timeout) {
  Status status;
  auto response = send_request();
  if (!response) {
    return Status::CLIENT_THRIFT_CALL_ERROR;
  }

  std::move(*(response.value()))
      .then([&status, &process_func, this](folly::Try<LaserResponse>&& t) { status = process_func(t); })
      .get();
  return status;
}

Status LaserClient::commonProcess(const folly::Try<LaserResponse>& t, ThriftResponseProcessFunc func,
                                  bool only_ok_call) {
  Status status = Status::OK;
  if (t.hasException()) {
    try {
      t.exception().throw_exception();
    }
    catch (const LaserException& ex) {
      status = ex.get_status();
    }
    catch (apache::thrift::transport::TTransportException& ex) {
      status = Status::CLIENT_THRIFT_CALL_TIMEOUT;
    }
    catch (apache::thrift::TApplicationException& ex) {
      status = Status::CLIENT_THRIFT_CALL_TIMEOUT;
    }
    catch (FutureTimeoutException& ex) {
      status = Status::CLIENT_THRIFT_FUTURE_TIMEOUT;
    }
    catch (...) {
      status = Status::CLIENT_THRIFT_CALL_ERROR;
    }
    if (!only_ok_call) {
      LaserResponse response;  // 此处不能调用 t.value(), 否则会抛异常
      func(response, status);  // 仅有 msetDetail / mgetDetail 才会要获取具体每个请求状态码
    }
  } else {
    status = func(t.value(), status);
  }

  if (status == Status::OK) {
    call_server_status_ok_->mark();
  } else {
    std::unordered_map<std::string, std::string> tags = {{"error", statusToName(status)}};
    metrics::Metrics::getInstance()
        ->buildMeter(LASER_CLIENT_MODULE_NAME, LASER_CLIENT_METRIC_CALL_SERVER_ERROR, tags)
        ->mark();
  }
  return status;
}

bool LaserClient::getRouteInfo(uint32_t* shard_id, int64_t* partition_hash, bool* route_to_edge_node,
                               const LaserKey& key, const ClientOption& options) {
  const std::string& database_name = key.get_database_name();
  const std::string& table_name = key.get_table_name();
  const std::vector<std::string>& primary_keys = key.get_primary_keys();
  const std::vector<std::string>& column_keys = key.get_column_keys();

  auto table_schema = config_manager_->getTableSchema(database_name, table_name);
  if (!table_schema) {
    return false;
  }
  laser::LaserKeyFormat format_key(primary_keys, column_keys);
  uint32_t partition_id = laser::PartitionManager::getPartitionId(database_name, table_name, format_key,
                                                                  table_schema.value()->getPartitionNumber());
  auto partition = std::make_shared<laser::Partition>(database_name, table_name, partition_id);
  auto op_shard_id = laser::PartitionManager::getShardId(partition, config_manager_);
  *shard_id = op_shard_id.hasValue() ? op_shard_id.value() : UINT32_MAX;
  *partition_hash = partition->getPartitionHash();
  if (*shard_id == UINT32_MAX || *partition_hash == 0) {
    return false;
  }
  *route_to_edge_node = false;
  if (options.getReadMode() != ClientRequestReadMode::LEADER_READ) {
    if (!table_schema.value()->getBindEdgeNodes().empty()) {
      uint32_t random_value = folly::Random::rand32(0, 100);
      if (random_value < table_schema.value()->getEdgeFlowRatio()) {
        *route_to_edge_node = true;
      }
    }
  }

  return true;
}

bool LaserClient::getRouteInfos(std::vector<std::tuple<uint32_t, int64_t, bool>>* route_infos,
                                const std::vector<LaserKey>& keys, const ClientOption& options) {
  for (auto& key : keys) {
    uint32_t shard_id = UINT32_MAX;
    int64_t partition_hash = 0;
    bool route_to_edge_node = false;
    bool result = getRouteInfo(&shard_id, &partition_hash, &route_to_edge_node, key, options);
    if (!result) {
      VLOG(10) << "Get database: " << key.get_database_name() << " table: " << key.get_table_name()
               << " route info failed!";
      return false;
    }
    route_infos->emplace_back(std::make_tuple(shard_id, partition_hash, route_to_edge_node));
  }
  return true;
}

std::shared_ptr<service_router::ClientOption> LaserClient::getClientOption(const ClientOption& options,
                                                                           uint32_t shard_id, int64_t partition_hash,
                                                                           bool route_to_edge_node) {
  auto option = std::make_shared<service_router::ClientOption>();
  option->setServiceName(target_service_name_);
  option->setProtocol(service_router::ServerProtocol::THRIFT);
  option->setShardId(shard_id);
  option->setPartitionHash(partition_hash);
  option->setRouteToEdgeNode(route_to_edge_node);
  option->setMaxConnPerServer(options.getMaxConnPerServer());
  option->setLoadBalance(options.getLoadBalance());
  option->setLocalFirstConfig(options.getLocalFirstConfig());
  option->setThriftCompressionMethod(options.getThriftCompressionMethod());
  option->setMaxConnPerServer(FLAGS_laser_client_max_conn_per_server);

  // 仅对单个操作的请求作用，mget mset 不会生效
  auto target_addr = options.getTargetServerAddress();
  if (target_addr.getPort() != 0 && !target_addr.getHost().empty()) {
    option->setTargetServerAddress(target_addr);
  }

  auto read_mode = options.getReadMode();
  switch (read_mode) {
    case ClientRequestReadMode::LEADER_READ:
      option->setShardType(service_router::ShardType::LEADER);
      break;
    case ClientRequestReadMode::FOLLOWER_READ:
      option->setShardType(service_router::ShardType::FOLLOWER);
      break;
    case ClientRequestReadMode::MIXED_READ:
      option->setShardType(service_router::ShardType::ALL);
      break;
    default:
      LOG(ERROR) << "Given read mode is invalid";
  }
  return option;
}

Status LaserClient::delSync(const ClientOption& options, const LaserKey& key) {
  metrics::Timer timer(del_command_timers_.get());
  return processSync([this, &key, &options]() { return del(options, key); },
                     [this](folly::Try<laser::LaserResponse>& t) { return okProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::del(const ClientOption& options, const LaserKey& key) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_delkey(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                       FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::expireSync(const ClientOption& options, const LaserKey& key, int64_t time) {
  metrics::Timer timer(expire_command_timers_.get());
  return processSync([this, &key, &options, time]() { return expire(options, key, time); },
                     [this](folly::Try<laser::LaserResponse>& t) { return okProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::expire(const ClientOption& options, const LaserKey& key,
                                                                     int64_t time) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, time, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_expire(rpc_options, key, time).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                             FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::expireAtSync(const ClientOption& options, const LaserKey& key, int64_t time_at) {
  metrics::Timer timer(expireat_command_timers_.get());
  return processSync([this, &key, &options, time_at]() { return expireAt(options, key, time_at); },
                     [this](folly::Try<laser::LaserResponse>& t) { return okProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::expireAt(const ClientOption& options, const LaserKey& key,
                                                                       int64_t time_at) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  bool ret = commonCall(key, write_options, [&response, &key, &options, time_at](auto client, auto& rpc_options) {
    auto future = client->future_expireAt(rpc_options, key, time_at)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::ttlSync(const ClientOption& options, int64_t* result, const LaserKey& key) {
  metrics::Timer timer(ttl_command_timers_.get());
  return processSync([this, &key, &options]() { return ttl(options, key); },
                     [this, result](folly::Try<laser::LaserResponse>& t) { return int64Process(result, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::ttl(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future = client->future_ttl(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::getSync(const ClientOption& options, std::string* data, const LaserKey& key) {
  metrics::Timer timer(get_command_timers_.get());
  return processSync([this, &key, &options]() { return get(options, key); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return getProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::get(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future = client->future_get(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::existSync(const ClientOption& options, bool* data, const LaserKey& key) {
  metrics::Timer timer(exist_command_timers_.get());
  return processSync([this, &key, &options]() { return exist(options, key); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return boolProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::exist(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_exist(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                      FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hsetSync(const ClientOption& options, const LaserKey& key, const std::string& field,
                             const std::string& value) {
  metrics::Timer timer(hset_command_timers_.get());
  return processSync([this, &key, &options, &field, &value]() { return hset(options, key, field, value); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hset(const ClientOption& options, const LaserKey& key,
                                                                   const std::string& field, const std::string& value) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret =
      commonCall(key, write_options, [&response, &key, &field, &value, &options](auto client, auto& rpc_options) {
        auto future = client->future_hset(rpc_options, key, field, value)
                          .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                  FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
        response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
      });

  if (ret) {
    return response;
  }
  return folly::none;
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hget(const ClientOption& options, const LaserKey& key,
                                                                   const std::string& field) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &field, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hget(rpc_options, key, field).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                            FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hgetSync(const ClientOption& options, std::string* data, const LaserKey& key,
                             const std::string& field) {
  metrics::Timer timer(hget_command_timers_.get());
  return processSync([this, &key, &options, &field]() { return hget(options, key, field); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return getProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hdel(const ClientOption& options, const LaserKey& key,
                                                                   const std::string& field) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &options, &key, &field](auto client, auto& rpc_options) {
    auto future =
        client->future_hdel(rpc_options, key, field).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                            FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hdelSync(const ClientOption& options, const LaserKey& key, const std::string& field) {
  metrics::Timer timer(hdel_command_timers_.get());
  return processSync([this, &key, &options, &field]() { return hdel(options, key, field); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hexists(const ClientOption& options, const LaserKey& key,
                                                                      const std::string& field) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &field, &options](auto client, auto& rpc_options) {
    auto future = client->future_hexists(rpc_options, key, field)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hexistsSync(const ClientOption& options, const LaserKey& key, const std::string& field) {
  metrics::Timer timer(hexists_command_timers_.get());
  return processSync([this, &key, &options, &field]() { return hexists(options, key, field); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hgetall(const ClientOption& options,
                                                                      const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hgetall(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                        FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hgetallSync(const ClientOption& options, std::map<std::string, std::string>* data,
                                const LaserKey& key) {
  metrics::Timer timer(hgetall_command_timers_.get());
  return processSync([this, &key, &options]() { return hgetall(options, key); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return mapProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hlen(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hlen(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hlenSync(const ClientOption& options, uint32_t* len, const LaserKey& key) {
  metrics::Timer timer(hlen_command_timers_.get());
  return processSync([this, &key, &options]() { return hlen(options, key); },
                     [this, len](folly::Try<laser::LaserResponse>& t) { return intProcess(len, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hmget(const ClientOption& options, const LaserKey& key,
                                                                    const std::vector<std::string>& fields) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &fields, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hmget(rpc_options, key, fields).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hmgetSync(const ClientOption& options, std::map<std::string, std::string>* data,
                              const LaserKey& key, const std::vector<std::string>& fields) {
  metrics::Timer timer(hmget_command_timers_.get());
  return processSync([this, &key, &options, &fields]() { return hmget(options, key, fields); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return mapProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hmset(const ClientOption& options, const LaserKey& key,
                                                                    const LaserValue& values) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &values, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hmset(rpc_options, key, values).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hmsetSync(const ClientOption& options, const LaserKey& key, const LaserValue& values) {
  metrics::Timer timer(hmset_command_timers_.get());
  return processSync([this, &key, &options, &values]() { return hmset(options, key, values); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::hkeys(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_hkeys(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                      FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::hkeysSync(const ClientOption& options, std::vector<std::string>* data, const LaserKey& key) {
  metrics::Timer timer(hkeys_command_timers_.get());
  return processSync([this, &key, &options]() { return hkeys(options, key); },
                     [this, data](folly::Try<laser::LaserResponse>& t) { return listProcess(data, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::lindex(const ClientOption& options, const LaserKey& key,
                                                                     uint32_t index) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, index, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_lindex(rpc_options, key, index).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::lindexSync(const ClientOption& options, std::string* item, const LaserKey& key, uint32_t index) {
  metrics::Timer timer(lindex_command_timers_.get());
  return processSync([this, &key, &options, index]() { return lindex(options, key, index); },
                     [this, item](folly::Try<laser::LaserResponse>& t) { return getProcess(item, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::llen(const ClientOption& options, const LaserKey& key) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_llen(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::llenSync(const ClientOption& options, uint32_t* len, const LaserKey& key) {
  metrics::Timer timer(llen_command_timers_.get());
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  return processSync([this, &key, &write_options]() { return llen(write_options, key); },
                     [this, len](folly::Try<laser::LaserResponse>& t) { return intProcess(len, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::lpop(const ClientOption& options, const LaserKey& key) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_lpop(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::lpopSync(const ClientOption& options, std::string* item, const LaserKey& key) {
  metrics::Timer timer(lpop_command_timers_.get());
  return processSync([this, &key, &options]() { return lpop(options, key); },
                     [this, item](folly::Try<laser::LaserResponse>& t) { return getProcess(item, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::rpop(const ClientOption& options, const LaserKey& key) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_rpop(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::rpopSync(const ClientOption& options, std::string* item, const LaserKey& key) {
  metrics::Timer timer(rpop_command_timers_.get());
  return processSync([this, &key, &options]() { return rpop(options, key); },
                     [this, item](folly::Try<laser::LaserResponse>& t) { return getProcess(item, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::lpush(const ClientOption& options, const LaserKey& key,
                                                                    const std::string& value) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &value, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_lpush(rpc_options, key, value).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                             FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::lpushSync(const ClientOption& options, const LaserKey& key, const std::string& value) {
  metrics::Timer timer(lpush_command_timers_.get());
  return processSync([this, &key, &value, &options]() { return lpush(options, key, value); },
                     [this](folly::Try<laser::LaserResponse>& t) { return okProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::rpush(const ClientOption& options, const LaserKey& key,
                                                                    const std::string& value) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &value, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_rpush(rpc_options, key, value).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                             FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::rpushSync(const ClientOption& options, const LaserKey& key, const std::string& value) {
  metrics::Timer timer(rpush_command_timers_.get());
  return processSync([this, &key, &value, &options]() { return rpush(options, key, value); },
                     [this](folly::Try<laser::LaserResponse>& t) { return okProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::lrange(const ClientOption& options, const LaserKey& key,
                                                                     uint32_t start, uint32_t end) {
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, start, end, &options](auto client, auto& rpc_options) {
    auto future = client->future_lrange(rpc_options, key, start, end)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::lrangeSync(const ClientOption& options, std::vector<std::string>* list, const LaserKey& key,
                               uint32_t start, uint32_t end) {
  metrics::Timer timer(lrange_command_timers_.get());
  return processSync([this, &key, &options, start, end]() { return lrange(options, key, start, end); },
                     [this, list](folly::Try<laser::LaserResponse>& t) { return listProcess(list, t); },
                     options.getReceiveTimeoutMs());
}

void LaserClient::mutilCallDispatchRequest(
    std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>>* address_to_indexs, const ClientOption& options,
    const std::vector<LaserKey>& keys) {
  call_server_timers_->mark();
  std::vector<std::tuple<uint32_t, int64_t, bool>> route_infos;
  if (!getRouteInfos(&route_infos, keys, options)) {
    return;
  }

  // 1. 一个请求使用 route_id 作为路由到目的 Server 的依据，route_id 可以是 shard_id, 也可以是 partition_hash,
  //    被分配到主集群的请求用 shard_id 作为 route_id，被分配到边缘节点的请求用 partition_hash 作为 route_id。
  // 2. 所以我们把所有的请求分成两部分: 一部分在主集群中进行服务发现，这部分请求的 route_id(shard_id)去重放到
  //    unique_shard_ids 中；另一部分在边缘节点中进行服务发现，这部分请求的 route_id(partition_hash)去重放到
  //    unique_partition_hashed 中。
  // 3. 服务发现的时候，分别使用 unique_shard_ids 作为 batchDiscover 参数去主集群进行服务发现、使用
  //    unqiue_partition_hashes 去边缘节点中进行服务发现。
  std::vector<int64_t> unique_shard_ids;
  std::vector<int64_t> unique_partition_hashes;
  std::unordered_map<int64_t, std::vector<uint32_t>> route_id_to_indexs;
  std::unordered_map<int64_t, uint32_t> partition_hash_to_shard_ids;
  for (size_t i = 0; i < route_infos.size(); ++i) {
    uint32_t shard_id = UINT32_MAX;
    int64_t partition_hash = 0;
    bool route_to_edge_node = false;
    std::tie(shard_id, partition_hash, route_to_edge_node) = route_infos[i];
    int64_t route_id = route_to_edge_node ? partition_hash : static_cast<int64_t>(shard_id);
    if (route_id_to_indexs.find(route_id) == route_id_to_indexs.end()) {
      route_id_to_indexs[route_id] = {};
      if (route_to_edge_node) {
        unique_partition_hashes.emplace_back(route_id);
        partition_hash_to_shard_ids[partition_hash] = shard_id;
      } else {
        unique_shard_ids.emplace_back(route_id);
      }
    }
    route_id_to_indexs[route_id].emplace_back(i);
  }

  // 服务发现的步骤：
  // 1. 被分配到边缘节点的请求从边缘节点中进行服务发现；
  // 2. 一部分在边缘节点中未发现到服务的请求，重新把它们分配到主集群中(把它们对应的 shard_id 放到 unique_shard_ids 中)。
  // 3. 剩下所有被分配到主集群的请求从主集群中进行服务发现。
  auto router = service_router::Router::getInstance();
  auto router_options = getClientOption(options);
  router->waitUntilDiscover(target_service_name_, service_router::FLAGS_discover_wait_milliseconds);
  // 从边缘节点进行服务发现
  std::unordered_map<int64_t, folly::Optional<service_router::Server>> edge_servers =
      router->batchDiscover(*router_options, unique_partition_hashes);
  // 找出从边缘节点服务发现失败的项，分配到主集群中
  for (auto it = edge_servers.begin(); it != edge_servers.end();) {
    if (!it->second.hasValue()) {
      if (partition_hash_to_shard_ids.find(it->first) == partition_hash_to_shard_ids.end()) {
        continue;
      }
      int64_t shard_id = static_cast<int64_t>(partition_hash_to_shard_ids[it->first]);
      if (route_id_to_indexs.find(shard_id) == route_id_to_indexs.end()) {
        route_id_to_indexs[shard_id] = {};
        unique_shard_ids.emplace_back(shard_id);
      }
      route_id_to_indexs[shard_id].insert(route_id_to_indexs[shard_id].end(), route_id_to_indexs[it->first].begin(),
                                          route_id_to_indexs[it->first].end());
      route_id_to_indexs.erase(it->first);
      it = edge_servers.erase(it);
    } else {
      ++it;
    }
  }

  // 从主集群中进行服务发现
  std::unordered_map<int64_t, folly::Optional<service_router::Server>> servers =
      router->batchDiscover(*router_options, unique_shard_ids);
  servers.insert(edge_servers.begin(), edge_servers.end());
  for (auto& server_info : servers) {
    if (!server_info.second.hasValue()) {
      continue;
    }

    uint64_t address_hash =
        CityHash64WithSeed(server_info.second.value().getHost().c_str(), server_info.second.value().getHost().size(),
                           server_info.second.value().getPort());
    if (address_to_indexs->find(address_hash) == address_to_indexs->end()) {
      auto address = std::make_shared<service_router::ServerAddress>();
      address->setHost(server_info.second.value().getHost());
      address->setPort(server_info.second.value().getPort());
      auto server_relation_keys = std::make_shared<ServerRelationKeys>(address);
      (*address_to_indexs)[address_hash] = server_relation_keys;
    }
    for (uint32_t index : route_id_to_indexs[server_info.first]) {
      (*address_to_indexs)[address_hash]->addIndex(index);
    }
  }
}

Status LaserClient::mget(const ClientOption& options, std::vector<LaserValue>* values,
                         const std::vector<LaserKey>& keys) {
  metrics::Timer timer(mget_command_timers_.get());
  std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>> address_to_indexs;
  mutilCallDispatchRequest(&address_to_indexs, options, keys);
  Status status = Status::OK;

  std::unordered_map<uint64_t, std::shared_ptr<std::vector<LaserKey>>> key_maps;
  std::unordered_map<uint32_t, uint64_t> package_indexs;

  // 默认值
  for (size_t i = 0; i < keys.size(); i++) {
    LaserValue value;
    value.set_null_value(true);
    values->push_back(std::move(value));
  }

  uint32_t package_index = 0;
  for (auto& index_info : address_to_indexs) {
    if (key_maps.find(index_info.first) == key_maps.end()) {
      key_maps[index_info.first] = std::make_shared<std::vector<LaserKey>>();
      package_indexs[package_index] = index_info.first;
      package_index++;
    }
    for (uint32_t index : index_info.second->getIndexes()) {
      key_maps[index_info.first]->push_back(keys[index]);
    }
  }

  std::vector<folly::Future<::laser::LaserResponse>> responses;
  for (uint32_t i = 0; i < package_index; i++) {
    auto address = address_to_indexs[package_indexs[i]]->getAddress();
    bool ret = callThriftServer(
        address, options,
        [&responses, &key_maps, address_hash = package_indexs[i], &options ](auto client, auto & rpc_options) {
          LaserKeys keys;
          keys.set_keys(std::move(*(key_maps[address_hash])));
          auto mget_future =
              client->future_mget(rpc_options, keys).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                            FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
          responses.push_back(std::move(mget_future));
        });
    if (!ret) {
      return Status::CLIENT_THRIFT_CALL_ERROR;
    }
  }

  folly::collectAll(responses)
      .via(work_thread_pool_.get())
      .thenValue([this, &address_to_indexs, values, &package_indexs](
           const std::vector<folly::Try<LaserResponse>>& try_responses) {
         for (size_t package_index = 0; package_index < try_responses.size(); package_index++) {
           if (package_indexs.find(package_index) == package_indexs.end()) {
             continue;
           }

           commonProcess(
               try_responses[package_index],
               [&address_to_indexs, values, address_hash = package_indexs[package_index] ](const LaserResponse & res,
                                                                                           const Status&)
                                                                                              ->Status {
                 if (address_to_indexs.find(address_hash) == address_to_indexs.end()) {
                   return Status::OK;
                 }
                 const std::vector<uint32_t>& indexs = address_to_indexs[address_hash]->getIndexes();
                 auto& list = res.get_list_value_data();
                 for (size_t i = 0; i < list.size(); i++) {
                   values->at(indexs[i]) = std::move(list[i]);
                 }
                 return Status::OK;
               },
               true);
         }
       })
      .get();

  return status;
}

Status LaserClient::mgetDetail(const ClientOption& options, std::vector<LaserValue>* values,
                               const std::vector<LaserKey>& keys) {
  metrics::Timer timer(mget_detail_command_timers_.get());
  std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>> address_to_indexs;
  mutilCallDispatchRequest(&address_to_indexs, options, keys);
  Status status = Status::OK;
  std::unordered_map<uint64_t, std::shared_ptr<std::vector<LaserKey>>> key_maps;
  std::unordered_map<uint32_t, uint64_t> package_indexs;
  // 默认值
  for (size_t i = 0; i < keys.size(); i++) {
    LaserValue value;
    EntryValue entry_value;
    entry_value.set_status(Status::UNKNOWN_ERROR);
    value.set_entry_value(entry_value);
    values->push_back(std::move(value));
  }
  uint32_t package_index = 0;
  for (auto& index_info : address_to_indexs) {
    if (key_maps.find(index_info.first) == key_maps.end()) {
      key_maps[index_info.first] = std::make_shared<std::vector<LaserKey>>();
      package_indexs[package_index] = index_info.first;
      package_index++;
    }
    for (uint32_t index : index_info.second->getIndexes()) {
      key_maps[index_info.first]->push_back(keys[index]);
    }
  }
  std::vector<folly::Future<::laser::LaserResponse>> responses;
  for (uint32_t i = 0; i < package_index; i++) {
    auto address = address_to_indexs[package_indexs[i]]->getAddress();
    bool ret = callThriftServer(
        address, options,
        [&responses, &key_maps, address_hash = package_indexs[i], &options ](auto client, auto & rpc_options) {
          LaserKeys keys;
          keys.set_keys(std::move(*(key_maps[address_hash])));
          auto mgetDetail_future = client->future_mgetDetail(rpc_options, keys)
                                       .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                               FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
          responses.push_back(std::move(mgetDetail_future));
        });
    if (!ret) {
      return Status::CLIENT_THRIFT_CALL_ERROR;
    }
  }

  folly::collectAll(responses)
      .via(work_thread_pool_.get())
      .thenValue([this, &address_to_indexs, values, &package_indexs](
           const std::vector<folly::Try<LaserResponse>>& try_responses) {
         for (size_t package_index = 0; package_index < try_responses.size(); package_index++) {
           if (package_indexs.find(package_index) == package_indexs.end()) {
             continue;
           }
           commonProcess(
               try_responses[package_index],
               [&address_to_indexs, values, address_hash = package_indexs[package_index] ](const LaserResponse & res,
                                                                                           const Status & status)
                                                                                              ->Status {
                 if (address_to_indexs.find(address_hash) == address_to_indexs.end()) {
                   return Status::OK;
                 }
                 const std::vector<uint32_t>& indexs = address_to_indexs[address_hash]->getIndexes();

                 if (status != Status::OK) {
                   for (size_t i = 0; i < indexs.size(); i++) {
                     LaserValue value;
                     EntryValue entry_value;
                     entry_value.set_status(status);
                     value.set_entry_value(entry_value);
                     values->at(indexs[i]) = std::move(value);
                   }
                   return Status::OK;
                 }

                 auto& list = res.get_list_value_data();
                 for (size_t i = 0; i < list.size(); i++) {
                   values->at(indexs[i]) = std::move(list[i]);
                 }
                 return Status::OK;
               },
               false);
         }
       })
      .get();

  int num_failed = 0;
  for (auto& val : *values) {
    Status entry_status = val.get_entry_value().get_status();
    if (Status::OK == entry_status || Status::RS_NOT_FOUND == entry_status || Status::RS_KEY_EXPIRE == entry_status) {
      continue;
    }
    num_failed++;
  }

  if (num_failed > 0) {
    status = (num_failed == values->size()) ? Status::RS_ERROR : Status::RS_PART_FAILED;
  }
  return status;
}

Status LaserClient::mdel(const ClientOption& options, std::vector<LaserValue>* values,
                         const std::vector<LaserKey>& keys) {
  metrics::Timer timer(mdel_command_timers_.get());
  std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>> address_to_indexs;
  mutilCallDispatchRequest(&address_to_indexs, options, keys);
  Status status = Status::OK;
  std::unordered_map<uint64_t, std::shared_ptr<std::vector<LaserKey>>> key_maps;
  std::unordered_map<uint32_t, uint64_t> package_indexs;
  // 默认值
  for (size_t i = 0; i < keys.size(); i++) {
    LaserValue value;
    EntryValue entry_value;
    entry_value.set_status(Status::UNKNOWN_ERROR);
    value.set_entry_value(entry_value);
    values->push_back(std::move(value));
  }
  uint32_t package_index = 0;
  for (auto& index_info : address_to_indexs) {
    if (key_maps.find(index_info.first) == key_maps.end()) {
      key_maps[index_info.first] = std::make_shared<std::vector<LaserKey>>();
      package_indexs[package_index] = index_info.first;
      package_index++;
    }
    for (uint32_t index : index_info.second->getIndexes()) {
      key_maps[index_info.first]->push_back(keys[index]);
    }
  }
  std::vector<folly::Future<::laser::LaserResponse>> responses;
  for (uint32_t i = 0; i < package_index; i++) {
    auto address = address_to_indexs[package_indexs[i]]->getAddress();
    bool ret = callThriftServer(
        address, options,
        [&responses, &key_maps, address_hash = package_indexs[i], &options ](auto client, auto & rpc_options) {
          LaserKeys keys;
          keys.set_keys(std::move(*(key_maps[address_hash])));
          auto mdel_future =
              client->future_mdel(rpc_options, keys).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                            FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
          responses.push_back(std::move(mdel_future));
        });
    if (!ret) {
      return Status::CLIENT_THRIFT_CALL_ERROR;
    }
  }

  folly::collectAll(responses)
      .via(work_thread_pool_.get())
      .thenValue([this, &address_to_indexs, values, &package_indexs](
           const std::vector<folly::Try<LaserResponse>>& try_responses) {
         for (size_t package_index = 0; package_index < try_responses.size(); package_index++) {
           if (package_indexs.find(package_index) == package_indexs.end()) {
             continue;
           }

           commonProcess(
               try_responses[package_index],
               [&address_to_indexs, values, address_hash = package_indexs[package_index] ](const LaserResponse & res,
                                                                                           const Status & status)
                                                                                              ->Status {
                 if (address_to_indexs.find(address_hash) == address_to_indexs.end()) {
                   return Status::OK;
                 }
                 const std::vector<uint32_t>& indexs = address_to_indexs[address_hash]->getIndexes();
                 if (status != Status::OK) {
                   for (size_t i = 0; i < indexs.size(); i++) {
                     LaserValue value;
                     EntryValue entry_value;
                     entry_value.set_status(status);
                     value.set_entry_value(entry_value);
                     values->at(indexs[i]) = std::move(value);
                   }
                   return Status::OK;
                 }
                 auto& list = res.get_list_value_data();
                 for (size_t i = 0; i < list.size(); i++) {
                   values->at(indexs[i]) = std::move(list[i]);
                 }
                 return Status::OK;
               },
               false);
         }
       })
      .get();

  int num_failed = 0;
  for (auto& val : *values) {
    Status entry_status = val.get_entry_value().get_status();
    if (Status::OK == entry_status || Status::RS_KEY_EXPIRE == entry_status) {
      continue;
    }
    num_failed++;
  }

  if (num_failed > 0) {
    status = (num_failed == values->size()) ? Status::RS_ERROR : Status::RS_PART_FAILED;
  }
  return status;
}

Status LaserClient::setSync(const ClientOption& options, const LaserKV& kv) {
  metrics::Timer timer(set_command_timers_.get());
  return processSync([this, &kv, &options]() { return set(options, kv); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::set(const ClientOption& options, const LaserKV& kv) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  const LaserKey& key = kv.get_key();
  bool ret = commonCall(key, write_options, [&response, &kv, &options](auto client, auto& rpc_options) {
    auto future = client->future_sset(rpc_options, kv).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::appendSync(const ClientOption& options, uint32_t* length, const LaserKey& key,
                               const std::string& value) {
  metrics::Timer timer(append_command_timers_.get());
  return processSync([this, &key, &value, &options]() { return append(options, key, value); },
                     [this, length](folly::Try<laser::LaserResponse>& t) { return intProcess(length, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::append(const ClientOption& options, const LaserKey& key,
                                                                     const std::string& value) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &value, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_append(rpc_options, key, value).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

Status LaserClient::setxSync(const ClientOption& options, const LaserKV& kv, const LaserSetOption& set_option) {
  metrics::Timer timer(setx_command_timers_.get());
  return processSync([this, &kv, &options, &set_option]() { return setx(options, kv, set_option); },
                     [this](folly::Try<laser::LaserResponse>& t) { return setProcess(t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::setx(const ClientOption& options, const LaserKV& kv,
                                                                   const LaserSetOption& set_option) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  const LaserKey& key = kv.get_key();
  bool ret = commonCall(key, write_options, [&response, &kv, &set_option, &options](auto client, auto& rpc_options) {
    auto future = client->future_setx(rpc_options, kv, set_option)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

Status LaserClient::mset(const ClientOption& options, std::vector<int64_t>* values, const std::vector<LaserKV>& kvs) {
  metrics::Timer timer(mset_command_timers_.get());
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  Status status = Status::OK;

  std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>> address_to_indexs;
  std::vector<LaserKey> keys;
  for (size_t i = 0; i < kvs.size(); i++) {
    // 默认值
    values->push_back(-1);
    keys.push_back(kvs[i].get_key());
  }
  mutilCallDispatchRequest(&address_to_indexs, write_options, keys);

  std::unordered_map<uint64_t, std::shared_ptr<std::vector<LaserKV>>> key_maps;
  std::unordered_map<uint32_t, uint64_t> package_indexs;

  uint32_t package_index = 0;
  for (auto& index_info : address_to_indexs) {
    if (key_maps.find(index_info.first) == key_maps.end()) {
      key_maps[index_info.first] = std::make_shared<std::vector<LaserKV>>();
      package_indexs[package_index] = index_info.first;
      package_index++;
    }
    for (uint32_t index : index_info.second->getIndexes()) {
      key_maps[index_info.first]->push_back(kvs[index]);
    }
  }

  std::vector<folly::Future<::laser::LaserResponse>> responses;
  for (uint32_t i = 0; i < package_index; i++) {
    auto address = address_to_indexs[package_indexs[i]]->getAddress();
    bool ret = callThriftServer(
        address, options,
        [&responses, &key_maps, address_hash = package_indexs[i], &options ](auto client, auto & rpc_options) {
          LaserKVs kvs;
          kvs.set_values(std::move(*(key_maps[address_hash])));
          auto mset_future =
              client->future_mset(rpc_options, kvs).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                           FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
          responses.push_back(std::move(mset_future));
        });
    if (!ret) {
      return Status::CLIENT_THRIFT_CALL_ERROR;
    }
  }

  folly::collectAll(responses)
      .via(work_thread_pool_.get())
      .thenValue([this, &address_to_indexs, values, &package_indexs](
           const std::vector<folly::Try<LaserResponse>>& try_responses) {
         for (size_t package_index = 0; package_index < try_responses.size(); package_index++) {
           if (package_indexs.find(package_index) == package_indexs.end()) {
             continue;
           }

           commonProcess(
               try_responses[package_index],
               [&address_to_indexs, values, address_hash = package_indexs[package_index] ](const LaserResponse & res,
                                                                                           const Status&)
                                                                                              ->Status {
                 if (address_to_indexs.find(address_hash) == address_to_indexs.end()) {
                   return Status::OK;
                 }
                 const std::vector<uint32_t>& indexs = address_to_indexs[address_hash]->getIndexes();
                 auto& list = res.get_list_int_data();
                 for (size_t i = 0; i < list.size(); i++) {
                   values->at(indexs[i]) = std::move(list[i]);
                 }
                 return Status::OK;
               },
               true);
         }
       })
      .get();
  return status;
}

Status LaserClient::msetDetail(const ClientOption& options, std::vector<LaserValue>* values,
                               const std::vector<LaserKV>& kvs) {
  LaserSetOption set_option;
  return msetDetail(options, values, kvs, set_option);
}

Status LaserClient::msetDetail(const ClientOption& options, std::vector<LaserValue>* values,
                               const std::vector<LaserKV>& kvs, const LaserSetOption& set_option) {
  metrics::Timer timer(mset_detail_command_timers_.get());
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  Status status = Status::OK;

  std::unordered_map<uint64_t, std::shared_ptr<ServerRelationKeys>> address_to_indexs;
  std::vector<LaserKey> keys;
  for (size_t i = 0; i < kvs.size(); i++) {
    // 默认值
    LaserValue value;
    EntryValue entry_value;
    entry_value.set_status(Status::UNKNOWN_ERROR);
    value.set_entry_value(entry_value);
    values->push_back(std::move(value));
    keys.push_back(kvs[i].get_key());
  }
  mutilCallDispatchRequest(&address_to_indexs, write_options, keys);

  std::unordered_map<uint64_t, std::shared_ptr<std::vector<LaserKV>>> key_maps;
  std::unordered_map<uint32_t, uint64_t> package_indexs;

  uint32_t package_index = 0;
  for (auto& index_info : address_to_indexs) {
    if (key_maps.find(index_info.first) == key_maps.end()) {
      key_maps[index_info.first] = std::make_shared<std::vector<LaserKV>>();
      package_indexs[package_index] = index_info.first;
      package_index++;
    }
    for (uint32_t index : index_info.second->getIndexes()) {
      key_maps[index_info.first]->push_back(kvs[index]);
    }
  }

  std::vector<folly::Future<::laser::LaserResponse>> responses;
  for (uint32_t i = 0; i < package_index; i++) {
    auto address = address_to_indexs[package_indexs[i]]->getAddress();
    bool ret = callThriftServer(
        address, options,
        [&responses, &key_maps, &set_option, address_hash = package_indexs[i], &options ](auto client,
                                                                                          auto & rpc_options) {
          LaserKVs kvs;
          kvs.set_values(std::move(*(key_maps[address_hash])));
          auto msetDetail_future = client->future_msetDetail(rpc_options, kvs, set_option)
                                       .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                               FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
          responses.push_back(std::move(msetDetail_future));
        });
    if (!ret) {
      return Status::CLIENT_THRIFT_CALL_ERROR;
    }
  }

  folly::collectAll(responses)
      .via(work_thread_pool_.get())
      .thenValue([this, &address_to_indexs, values, &package_indexs](
           const std::vector<folly::Try<LaserResponse>>& try_responses) {
         for (size_t package_index = 0; package_index < try_responses.size(); package_index++) {
           if (package_indexs.find(package_index) == package_indexs.end()) {
             continue;
           }

           commonProcess(
               try_responses[package_index],
               [&address_to_indexs, values, address_hash = package_indexs[package_index] ](const LaserResponse & res,
                                                                                           const Status & status)
                                                                                              ->Status {
                 if (address_to_indexs.find(address_hash) == address_to_indexs.end()) {
                   return Status::OK;
                 }
                 const std::vector<uint32_t>& indexs = address_to_indexs[address_hash]->getIndexes();
                 if (status != Status::OK) {
                   for (size_t i = 0; i < indexs.size(); i++) {
                     LaserValue value;
                     EntryValue entry_value;
                     entry_value.set_status(status);
                     value.set_entry_value(entry_value);
                     values->at(indexs[i]) = std::move(value);
                   }
                   return Status::OK;
                 }
                 auto& list = res.get_list_value_data();
                 for (size_t i = 0; i < list.size(); i++) {
                   values->at(indexs[i]) = std::move(list[i]);
                 }
                 return Status::OK;
               },
               false);
         }
       })
      .get();

  int num_failed = 0;
  for (auto& val : *values) {
    Status entry_status = val.get_entry_value().get_status();
    if (Status::OK == entry_status || Status::RS_KEY_EXPIRE == entry_status) {
      continue;
    }
    num_failed++;
  }

  if (num_failed > 0) {
    status = (num_failed == values->size()) ? Status::RS_ERROR : Status::RS_PART_FAILED;
  }
  return status;
}

Status LaserClient::decrSync(const ClientOption& options, int64_t* result, const LaserKey& key) {
  metrics::Timer timer(set_command_timers_.get());
  return processSync([this, &key, &options]() { return decr(options, key); },
                     [this, result](folly::Try<laser::LaserResponse>& t) { return int64Process(result, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::decr(const ClientOption& options, const LaserKey& key) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_decr(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

Status LaserClient::incrSync(const ClientOption& options, int64_t* result, const LaserKey& key) {
  metrics::Timer timer(set_command_timers_.get());
  return processSync([this, &key, &options]() { return incr(options, key); },
                     [this, result](folly::Try<laser::LaserResponse>& t) { return int64Process(result, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::incr(const ClientOption& options, const LaserKey& key) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_incr(rpc_options, key).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                     FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

Status LaserClient::decrBySync(const ClientOption& options, int64_t* result, const LaserKey& key, int64_t step) {
  metrics::Timer timer(set_command_timers_.get());
  return processSync([this, &key, step, &options]() { return decrBy(options, key, step); },
                     [this, result](folly::Try<laser::LaserResponse>& t) { return int64Process(result, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::decrBy(const ClientOption& options, const LaserKey& key,
                                                                     int64_t step) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, step, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_decrBy(rpc_options, key, step).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                             FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

Status LaserClient::incrBySync(const ClientOption& options, int64_t* result, const LaserKey& key, int64_t step) {
  metrics::Timer timer(set_command_timers_.get());
  return processSync([this, &key, step, &options]() { return incrBy(options, key, step); },
                     [this, result](folly::Try<laser::LaserResponse>& t) { return int64Process(result, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::incrBy(const ClientOption& options, const LaserKey& key,
                                                                     int64_t step) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, write_options, [&response, &key, step, &options](auto client, auto& rpc_options) {
    auto future =
        client->future_incrBy(rpc_options, key, step).within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                                             FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }

  return folly::none;
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::zadd(
    const ClientOption& options, const LaserKey& key, const std::unordered_map<std::string, double>& member_scores) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  LaserValue laser_values;
  std::map<std::string, int64_t> member_int_score;
  for (auto member_score : member_scores) {
    double zoom_score = member_score.second * LASER_FLOAT_AMPLIFICATION_FACTOR;
    int64_t score = zoom_score > INT64_MAX ? INT64_MAX : zoom_score;
    member_int_score.insert({member_score.first, score});
    VLOG(5) << "laser client zadd the key is:" << key.get_primary_keys()[0] << " member is:" << member_score.first
            << " the socre is:" << score;
  }
  laser_values.set_member_score_value(member_int_score);
  bool ret = commonCall(key, write_options, [&response, &key, &laser_values, &options](auto client, auto& rpc_options) {
    auto future = client->future_zadd(rpc_options, key, laser_values)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::zaddSync(const ClientOption& options, uint32_t* res, const LaserKey& key,
                             const std::unordered_map<std::string, double>& member_scores) {
  metrics::Timer timer(zadd_command_timers_.get());

  return processSync([this, &key, &options, &member_scores]() { return zadd(options, key, member_scores); },
                     [this, res](folly::Try<laser::LaserResponse>& t) { return intProcess(res, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::zrangebyscore(const ClientOption& options,
                                                                            const LaserKey& key, double min,
                                                                            double max) {
  int64_t int_min;
  int64_t int_max;
  getIntMinMax(&int_min, &int_max, min, max);
  VLOG(5) << "laser client zrangebyscore the key is:" << key.get_primary_keys()[0] << " the min is:" << int_min
          << " the max is:" << int_max;
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  bool ret = commonCall(key, options, [&response, &key, int_min, int_max, &options](auto client, auto& rpc_options) {
    auto future = client->future_zrangeByScore(rpc_options, key, int_min, int_max)
                      .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                              FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
    response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
  });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::zrangebyscoreSync(const ClientOption& options, std::vector<LaserFloatScoreMember>* res,
                                      const LaserKey& key, double min, double max) {
  metrics::Timer timer(zrangebyscore_command_timers_.get());
  return processSync([this, &key, &options, min, max]() { return zrangebyscore(options, key, min, max); },
                     [this, res](folly::Try<laser::LaserResponse>& t) { return scoreMemberProcess(res, t); },
                     options.getReceiveTimeoutMs());
}

folly::Optional<std::shared_ptr<FutureResponse>> LaserClient::zremrangebyscore(const ClientOption& options,
                                                                               const LaserKey& key, double min,
                                                                               double max) {
  ClientOption write_options = options;
  write_options.setReadMode(ClientRequestReadMode::LEADER_READ);
  std::shared_ptr<folly::Future<::laser::LaserResponse>> response;
  int64_t int_min;
  int64_t int_max;
  getIntMinMax(&int_min, &int_max, min, max);
  bool ret =
      commonCall(key, write_options, [&response, &key, int_min, int_max, &options](auto client, auto& rpc_options) {
        auto future = client->future_zremRangeByScore(rpc_options, key, int_min, int_max)
                          .within(std::chrono::milliseconds(options.getReceiveTimeoutMs()),
                                  FutureTimeoutException(FUTURE_TIMEOUT_EXCEPTION_MESSAGE));
        response = std::make_shared<folly::Future<::laser::LaserResponse>>(std::move(future));
      });

  if (ret) {
    return response;
  }
  return folly::none;
}

Status LaserClient::zremrangebyscoreSync(const ClientOption& options, uint32_t* res, const LaserKey& key, double min,
                                         double max) {
  metrics::Timer timer(zremrangebyscore_command_timers_.get());
  return processSync([this, &key, &options, min, max]() { return zremrangebyscore(options, key, min, max); },
                     [this, res](folly::Try<laser::LaserResponse>& t) { return intProcess(res, t); },
                     options.getReceiveTimeoutMs());
}

void LaserClient::getResult(std::shared_ptr<FutureResponse> response, GetResponseProcessFunc process_func) {
  std::move(*response).via(work_thread_pool_.get())
      .then([&process_func](folly::Try<LaserResponse>&& t) { process_func(t); })
      .get();
}

void LaserClient::collectAllResult(const std::vector<std::shared_ptr<FutureResponse>>& responses,
                                   CollectAllResponseProcessFunc process_func) {
  std::vector<FutureResponse> responses_vecs;
  for (auto res : responses) {
    responses_vecs.push_back(std::move(*res));
  }
  folly::collectAll(responses_vecs)
      .via(work_thread_pool_.get())
      .thenValue([&process_func](const std::vector<folly::Try<LaserResponse>>&& try_responses) {
         process_func(try_responses);
       })
      .get();
}

Status LaserClient::getProcess(std::string* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::string_data == response.getType()) {
                              *data = std::move(response.get_string_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::boolProcess(bool* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::bool_data == response.getType()) {
                              *data = std::move(response.get_bool_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::mapProcess(std::map<std::string, std::string>* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::map_string_data == response.getType()) {
                              *data = std::move(response.get_map_string_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::listProcess(std::vector<std::string>* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::list_string_data == response.getType()) {
                              *data = std::move(response.get_list_string_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::scoreMemberProcess(std::vector<LaserFloatScoreMember>* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::list_score_member_data == response.getType()) {
                              auto int_score_members = response.get_list_score_member_data();
                              for (auto int_score_member : int_score_members) {
                                LaserFloatScoreMember double_score_member;
                                double_score_member.setMember(int_score_member.get_member());
                                double double_score = int_score_member.get_score();
                                double_score_member.setScore(double_score / LASER_FLOAT_AMPLIFICATION_FACTOR);
                                data->push_back(double_score_member);
                              }
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::intProcess(uint32_t* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::int_data == response.getType()) {
                              *data = std::move(response.get_int_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}

Status LaserClient::int64Process(int64_t* data, const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [data](auto& response, const Status&) {
                            if (LaserResponse::Type::int_data == response.getType()) {
                              *data = std::move(response.get_int_data());
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                            return Status::OK;
                          },
                       true);
}
Status LaserClient::setProcess(const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [](auto& response, const Status&) {
                            if (LaserResponse::Type::int_data == response.getType() && response.get_int_data() >= 0) {
                              return Status::OK;
                            } else {
                              return Status::SERVICE_UNION_DATA_TYPE_INVALID;
                            }
                          },
                       true);
}

Status LaserClient::okProcess(const folly::Try<LaserResponse>& t) {
  return commonProcess(t, [](auto& response, const Status&) { return Status::OK; }, true);
}

void LaserClient::getIntMinMax(int64_t* int_min, int64_t* int_max, double double_min, double double_max) {
  double ap_min = double_min * LASER_FLOAT_AMPLIFICATION_FACTOR;
  double ap_max = double_max * LASER_FLOAT_AMPLIFICATION_FACTOR;
  *int_min = ap_min < INT64_MIN ? INT64_MIN : ap_min;
  *int_max = ap_max > INT64_MAX ? INT64_MAX : ap_max;
}

}  // namespace laser
