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
 * @author liubang <it.liubang@gmail.com>
 */

#include "folly/init/Init.h"
#include "folly/system/ThreadName.h"
#include "gflags/gflags.h"

#include "common/service_router/router.h"
#include "common/service_router/thrift.h"
#include "common/service_router/http.h"
#include "common/laser/config_manager.h"

#include "http_service.h"
#include "engine/rocksdb.h"
#include "laser_service.h"
#include "database_manager.h"

DEFINE_string(service_name, "laser_dev", "Laser Server service name");
DEFINE_string(replicator_service_name, "laser_dev_replicator", "Laser Server service name");
DEFINE_string(group_name, "aliyun", "Laser cluster group name");
DEFINE_string(dc, "default", "Laser cluster data center name");
DEFINE_int32(node_id, 1, "Laser server node id");
DEFINE_string(host, "127.0.0.1", "Laser server host");
DEFINE_int32(port, 0, "Laser server port");
DEFINE_int32(http_port, 10010, "Laser server http port");
DEFINE_int32(replicator_port, 0, "Laser server replicator port");
DEFINE_int32(loader_thread_nums, 4, "Laser server loader thread number");

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  // 配置管理、数据引擎管理
  // 必须在 database_manager 后面初始化
  auto config_manager = std::make_shared<laser::ConfigManager>(service_router::Router::getInstance());
  auto database_manager = std::make_shared<laser::DatabaseManager>(config_manager, FLAGS_group_name, FLAGS_node_id, FLAGS_dc);
  database_manager->init(FLAGS_loader_thread_nums, FLAGS_replicator_service_name, FLAGS_host, FLAGS_replicator_port);
  config_manager->init(FLAGS_service_name, FLAGS_group_name, FLAGS_node_id);

  // http 服务初始化
  laser::HttpService http_service(database_manager);
  http_service.init();
  std::thread http_server_thread([]() {
    folly::setThreadName("httpServerStart");
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_http_port, folly::none, 0,
                                      service_router::ServerStatus::UNAVAILABLE);
  });

  // thrift 初始化
  std::shared_ptr<laser::LaserService> handler =
      std::make_shared<laser::LaserService>(config_manager, database_manager);
  auto thrift_server_modifier = [](service_router::ThriftServer&) {};
  auto server_on_create = [database_manager](const service_router::Server& server) {
      service_router::Router::getInstance()->setDc(server, FLAGS_dc);
    // 注册到 database manager 中用来控制服务状态等信息
    database_manager->setServiceServer(server);
  };
  service_router::thriftServiceServer<laser::LaserService>(FLAGS_service_name, FLAGS_host, FLAGS_port, handler,
                                                           thrift_server_modifier, 0,
                                                           service_router::ServerStatus::UNAVAILABLE, server_on_create);
  http_server_thread.join();
  return 0;
}
