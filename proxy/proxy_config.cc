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

#include "proxy_config.h"

namespace laser {

// consul : services
constexpr static char LASER_PROXY_CONFIG_DB_TABLE_CONFIG_LIST[] = "table_config";

void ProxyConfig::init(const std::string& service_name) {
  auto router = router_.lock();
  if (!router) {
    LOG(ERROR) << "init router failed.";
    return;
  }
  router->waitForConfig(service_name);
  auto config_vals = router->getConfigs(service_name);
  updateConfig(config_vals);
  router->subscribeConfig(service_name, [this](const std::string&, const service_router::ServiceConfig& config) {
    updateConfig(config.getConfigs());
  });
}

void ProxyConfig::updateConfig(const std::unordered_map<std::string, std::string>& configs) {
  for (auto& config : configs) {
    if (config.first != LASER_PROXY_CONFIG_DB_TABLE_CONFIG_LIST) {
      continue;
    }
    parserProxyTableConfig(config.second);
  }
}

void ProxyConfig::parserProxyTableConfig(const std::string& proxy_consul_table_config_value) {
  folly::dynamic dy_proxy_table_config;
  if (!common::fromJson(&dy_proxy_table_config, proxy_consul_table_config_value)) {
    LOG(ERROR) << "Json parse proxy db table config list failed, value: " << proxy_consul_table_config_value;
    return;
  }
  auto proxy_table_config = std::make_shared<ProxyTableConfig>();
  if (!proxy_table_config->deserialize(dy_proxy_table_config)) {
    LOG(ERROR) << "Proxy Db Table config list deserialize failed, value: " << proxy_consul_table_config_value;
    return;
  }

  update(proxy_table_config);
}

void ProxyConfig::update(std::shared_ptr<ProxyTableConfig> table_config) {
  std::unordered_map<uint64_t, ProxySpecificTableConfig> proxy_table_config;
  auto consul_config = table_config->getProxyTableConfig();
  auto database_table_configs = consul_config.getProxyDbTableConfigList();
  for (auto& table_config : database_table_configs) {
    for (auto& table_info : table_config.second.getProxyDbTableConfig()) {
      uint64_t table_key = getTableSchemaHash(table_config.first, table_info.first);
      proxy_table_config[table_key] = table_info.second;
    }
  }

  proxy_table_config_.withWLock([&proxy_table_config](auto& configs) {
    configs = proxy_table_config;
  });
  auto read_timeout  = table_config->getLaserClientReadTimeout();
  auto write_timeout = table_config->getLaserClientWriteTimeout();
  auto allowed_flow  = table_config->getLaserClientAllowedFlow();
  default_read_timeout_  = (read_timeout == 0)  ? default_read_timeout_  : read_timeout;
  default_write_timeout_ = (write_timeout == 0) ? default_write_timeout_ : write_timeout;
  default_allowed_flow_  = (allowed_flow == 0)  ? default_allowed_flow_  : allowed_flow;
  LOG(INFO) << "update success, current default_read_timeout_ : " << default_read_timeout_ <<
               " , default_write_timeout_ : " << default_write_timeout_ <<
               " , default_allowed_flow_ : " << default_allowed_flow_;
}

int32_t ProxyConfig::getReadTimeout(const std::string &database_name, const std::string &table_name) {
  // 获取表级
  auto key = getTableSchemaHash(database_name, table_name);
  auto table_config = proxy_table_config_.withRLock([this](auto& value) { return value; });
  auto it = table_config.find(key);
  if (it != table_config.end()) {
    return table_config[key].getTableReadTimeout();
  } else {
    return default_read_timeout_;
  }
}
int32_t ProxyConfig::getWriteTimeout(const std::string &database_name, const std::string &table_name) {
  // 获取表级
  auto key = getTableSchemaHash(database_name, table_name);
  auto table_config = proxy_table_config_.withRLock([this](auto& value) { return value; });
  auto it = table_config.find(key);
  if (it != table_config.end()) {
    return table_config[key].getTableWriteTimeout();
  } else {
    return default_write_timeout_;
  }
}

int32_t ProxyConfig::getAllowedFlow(const std::string &database_name, const std::string &table_name) {
  // 获取表级
  auto key = getTableSchemaHash(database_name, table_name);
  auto table_config = proxy_table_config_.withRLock([this](auto& value) { return value; });
  auto it = table_config.find(key);
  if (it != table_config.end()) {
    return table_config[key].getTableAllowedFlow();
  } else {
    return default_allowed_flow_;
  }
}

uint64_t ProxyConfig::getTableSchemaHash(const std::string& database_name, const std::string& table_name) {
  uint64_t key = CityHash64WithSeed(database_name.data(), database_name.size(), 0);
  return CityHash64WithSeed(table_name.data(), table_name.size(), key);
}

}  // namespace laser
