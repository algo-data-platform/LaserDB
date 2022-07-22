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
 */

#pragma once

#include "common/http/http_server_manager.h"
#include "database_manager.h"

namespace laser {

class HttpService {
 public:
  explicit HttpService(std::shared_ptr<DatabaseManager> database_manager) : database_manager_(database_manager) {}
  ~HttpService() = default;
  void init();

 private:
  std::shared_ptr<DatabaseManager> database_manager_;

  void loadBase(service_framework::http::ServerResponse* response, const std::string& database_name,
                const std::string& table_name, const std::string& version);
  void loadDelta(service_framework::http::ServerResponse* response, const std::string& database_name,
                 const std::string& table_name, const std::string& version, const std::string& delta_version_str);
  void baseDataReplication(service_framework::http::ServerResponse* response, const std::string& database_name,
                           const std::string& table_name);
  void dbMetadata(service_framework::http::ServerResponse* response, const std::string& database_name,
                  const std::string& table_name);
  void shardList(service_framework::http::ServerResponse* response);
  void shardUnavailable(service_framework::http::ServerResponse* response, const std::string& request_body);
  void monitorSwitch(service_framework::http::ServerResponse* response, const std::string& switch_flag);
  void cleanPartitions(service_framework::http::ServerResponse* response);
  void updateConfigs(service_framework::http::ServerResponse* response, const std::string& request_body);
  void resultJson(service_framework::http::ServerResponse* response, const folly::dynamic& data);
  void errorJson(service_framework::http::ServerResponse* response, uint32_t code, const std::string& error);
  void sendResponse(service_framework::http::ServerResponse* response, uint32_t code, const std::string& error,
                    const folly::dynamic& data);
};

}  // namespace laser
