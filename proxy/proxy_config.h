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
 * @author Mingrui Zhang <zmr13140@gmail.com>
 * @author Junpeng Liu <liujunpeng555@gmail.com>
 */

#pragma once

#include "folly/SpinLock.h"
#include "folly/Synchronized.h"

#include "common/service_router/router.h"
#include "common/laser/laser_entity.h"

DECLARE_int32(laser_client_read_timeout);
DECLARE_int32(laser_client_write_timeout);
DECLARE_int32(laser_client_allowed_flow);

namespace laser {

class ProxyConfig {
 public:
  explicit ProxyConfig(const std::shared_ptr<service_router::Router> router) : router_(router) {}
  virtual ~ProxyConfig() = default;
  virtual void init(const std::string& service_name);
  virtual void updateConfig(const std::unordered_map<std::string, std::string>& configs);
  virtual void parserProxyTableConfig(const std::string& proxy_consul_table_config_value);
  virtual void update(std::shared_ptr<ProxyTableConfig> table_config);
  virtual int32_t getReadTimeout(const std::string &database_name, const std::string &table_name);
  virtual int32_t getWriteTimeout(const std::string &database_name, const std::string &table_name);
  virtual int32_t getAllowedFlow(const std::string &database_name, const std::string &table_name);
  virtual uint64_t getTableSchemaHash(const std::string& database_name, const std::string& table_name);

 private:
  std::weak_ptr<service_router::Router> router_;
  int32_t default_read_timeout_{FLAGS_laser_client_read_timeout};
  int32_t default_write_timeout_{FLAGS_laser_client_write_timeout};
  int32_t default_allowed_flow_{FLAGS_laser_client_allowed_flow};
  folly::Synchronized<std::unordered_map<uint64_t, ProxySpecificTableConfig>> proxy_table_config_;
};

}  // namespace laser
