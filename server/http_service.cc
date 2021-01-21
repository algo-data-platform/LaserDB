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

#include "http_service.h"
#include "database_manager.h"

namespace laser {

constexpr static char HTTP_SERVICE_URL_UPDATE_BASE_DATA[] = "/update/base";
constexpr static char HTTP_SERVICE_URL_UPDATE_DELTA_DATA[] = "/update/delta";
constexpr static char HTTP_SERVICE_URL_GET_META_DATA[] = "/db/meta";
constexpr static char HTTP_SERVICE_URL_SHARD_LIST[] = "/shard/list";
constexpr static char HTTP_SERVICE_URL_SHARD_UNAVAILABLE[] = "/shard/unavailable";
constexpr static char HTTP_SERVICE_URL_CLEAN_PARTITIONS[] = "/clean/partitions";
constexpr static char HTTP_SERVICE_URL_MONITOR_SWITCH_UNAVAILABLE[] = "/monitor/switch";
constexpr static char HTTP_SERVICE_URL_UPDATE_CONFIGS[] = "/update/configs";
constexpr static char HTTP_SERVICE_URL_UPDATE_BASE_DATA_REPLICATION[] = "/update/base_replication";

constexpr static char MONITOR_SWITCH_FLAG_ENABLE[] = "enable";
constexpr static char MONITOR_SWITCH_FLAG_DISABLE[] = "disable";
constexpr static char PARAM_CONFIG_NAME[] = "config_name";
constexpr static char PARAM_CONFIG_DATA[] = "config_data";

void HttpService::init() {
  auto server_manager = service_framework::http::HttpServerManager::getInstance();
  server_manager->registerLocation(
      HTTP_SERVICE_URL_UPDATE_BASE_DATA,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) {
        const std::string& database_name = request->getQueryParam("database_name");
        const std::string& table_name = request->getQueryParam("table_name");
        const std::string& version = request->getQueryParam("version");
        loadBase(response, database_name, table_name, version);
      });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_UPDATE_DELTA_DATA,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) {
        const std::string& database_name = request->getQueryParam("database_name");
        const std::string& table_name = request->getQueryParam("table_name");
        const std::string& version = request->getQueryParam("version");
        const std::string& delta_version_str = request->getQueryParam("delta_versions");
        loadDelta(response, database_name, table_name, version, delta_version_str);
      });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_UPDATE_BASE_DATA_REPLICATION,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) {
        const std::string& database_name = request->getQueryParam("database_name");
        const std::string& table_name = request->getQueryParam("table_name");
        baseDataReplication(response, database_name, table_name);
      });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_GET_META_DATA,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) {
        const std::string& database_name = request->getQueryParam("database_name");
        const std::string& table_name = request->getQueryParam("table_name");
        dbMetadata(response, database_name, table_name);
      });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_SHARD_LIST,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) { shardList(response); });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_SHARD_UNAVAILABLE,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage>,
             std::unique_ptr<folly::IOBuf> request_body) {
        const folly::IOBuf* p = request_body.get();
        std::string body;
        if (p == nullptr) {
          errorJson(response, 10004, "Shard list data is empty.");
          return;
        }
        do {
          body.append(reinterpret_cast<const char*>(p->data()), p->length());
          p = p->next();
        } while (p != request_body.get());
        shardUnavailable(response, body);
      },
      proxygen::HTTPMethod::POST);
  server_manager->registerLocation(
      HTTP_SERVICE_URL_CLEAN_PARTITIONS,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage>,
             std::unique_ptr<folly::IOBuf>) { cleanPartitions(response); },
      proxygen::HTTPMethod::POST);
  server_manager->registerLocation(
      HTTP_SERVICE_URL_MONITOR_SWITCH_UNAVAILABLE,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage> request,
             std::unique_ptr<folly::IOBuf>) {
        const std::string& switch_flag_string = request->getQueryParam("switch_flag");
        monitorSwitch(response, switch_flag_string);
      });
  server_manager->registerLocation(
      HTTP_SERVICE_URL_UPDATE_CONFIGS,
      [this](service_framework::http::ServerResponse* response, std::unique_ptr<proxygen::HTTPMessage>,
             std::unique_ptr<folly::IOBuf> request_body) {
        const folly::IOBuf* p = request_body.get();
        std::string body;
        if (p == nullptr) {
          errorJson(response, 10006, "Config data is empty");
          return;
        }
        do {
          body.append(reinterpret_cast<const char*>(p->data()), p->length());
          p = p->next();
        } while (p != request_body.get());
        updateConfigs(response, body);
      },
      proxygen::HTTPMethod::POST);
}

void HttpService::loadBase(service_framework::http::ServerResponse* response, const std::string& database_name,
                           const std::string& table_name, const std::string& version) {
  folly::dynamic null;
  database_manager_->triggerBase(database_name, table_name, version);
  resultJson(response, null);
}

void HttpService::loadDelta(service_framework::http::ServerResponse* response, const std::string& database_name,
                            const std::string& table_name, const std::string& version,
                            const std::string& delta_version_str) {
  if (delta_version_str.empty()) {
    errorJson(response, 10001, "delta version is invalid");
    return;
  }
  std::vector<std::string> delta_versions;
  folly::split(',', delta_version_str, delta_versions);

  if (delta_versions.empty()) {
    errorJson(response, 10001, "delta version is invalid");
    return;
  }
  database_manager_->triggerDelta(database_name, table_name, version, delta_versions);
  folly::dynamic null;
  resultJson(response, null);
}

void HttpService::baseDataReplication(service_framework::http::ServerResponse* response,
                                      const std::string& database_name, const std::string& table_name) {
  folly::dynamic null;
  database_manager_->triggerBaseDataReplication(database_name, table_name);
  resultJson(response, null);
}

void HttpService::dbMetadata(service_framework::http::ServerResponse* response, const std::string& database_name,
                             const std::string& table_name) {
  TableMetaInfo table_meta_info;
  database_manager_->getTableMetaInfo(&table_meta_info, database_name, table_name);
  folly::dynamic dy_table_meta_info = table_meta_info.serialize();
  resultJson(response, dy_table_meta_info);
}

void HttpService::shardList(service_framework::http::ServerResponse* response) {
  std::vector<std::shared_ptr<ShardMetaInfo>> shards;
  database_manager_->getShardMetaInfo(&shards);
  folly::dynamic dy_shards = folly::dynamic::array;
  for (auto& shard : shards) {
    dy_shards.push_back(shard->serialize());
  }
  resultJson(response, dy_shards);
}

void HttpService::shardUnavailable(service_framework::http::ServerResponse* response, const std::string& request_body) {
  folly::dynamic shards;
  if (!common::fromJson(&shards, request_body)) {
    errorJson(response, 10002, "Shard list invalid.");
    return;
  }

  if (!shards.isArray()) {
    errorJson(response, 10003, "Shard list invalid.");
    return;
  }
  std::vector<uint32_t> shard_ids;
  for (size_t i = 0; i < shards.size(); i++) {
    if (!shards.at(i).isInt()) {
      errorJson(response, 10004, "Shard id type invalid.");
      return;
    }
    shard_ids.push_back(shards.at(i).asInt());
  }

  database_manager_->setUnavailableShards(shard_ids);
  resultJson(response, shards);
}

void HttpService::monitorSwitch(service_framework::http::ServerResponse* response,
                                const std::string& switch_flag_string) {
  folly::dynamic null;

  bool switch_flag_bool = true;

  if (switch_flag_string == MONITOR_SWITCH_FLAG_ENABLE) {
    switch_flag_bool = true;
    LOG(INFO) << "Switch flag is true.";
  } else if (switch_flag_string == MONITOR_SWITCH_FLAG_DISABLE) {
    switch_flag_bool = false;
    LOG(INFO) << "Swtich flag is false.";
  } else {
    errorJson(response, 10005, "Switch flag invalid.");
    return;
  }

  database_manager_->monitorSwitch(switch_flag_bool);
  resultJson(response, null);
}

void HttpService::cleanPartitions(service_framework::http::ServerResponse* response) {
  auto deleted_partitions = std::make_shared<std::vector<PartitionMetaInfo>>();
  database_manager_->cleanUnusedPartitions(deleted_partitions);
  folly::dynamic dy_partitions = folly::dynamic::array;
  for (auto& partition : *deleted_partitions) {
    dy_partitions.push_back(partition.serialize());
  }
  resultJson(response, dy_partitions);
}

void HttpService::updateConfigs(service_framework::http::ServerResponse* response, const std::string& request_body) {
  bool data_parse_ok = false;
  std::string config_name = "";
  std::string config_data = "";
  do {
    std::vector<std::string> param_key_value_strs;
    folly::split('&', request_body, param_key_value_strs);
    std::unordered_map<std::string, std::string> param_key_values;
    for (auto& key_val_str : param_key_value_strs) {
      std::vector<std::string> key_val;
      folly::split('=', key_val_str, key_val);
      if (key_val.size() != 2) {
        continue;
      }
      param_key_values.insert(std::make_pair(key_val[0], key_val[1]));
    }
    auto config_name_pair = param_key_values.find(PARAM_CONFIG_NAME);
    if (config_name_pair == param_key_values.end()) {
      break;
    }
    config_name = config_name_pair->second;

    auto config_data_pair = param_key_values.find(PARAM_CONFIG_DATA);
    if (config_data_pair == param_key_values.end()) {
      break;
    }
    config_data = config_data_pair->second;
    data_parse_ok = true;
  } while (false);

  if (!data_parse_ok) {
    LOG(ERROR) << "Param is invalid: " << request_body;
    errorJson(response, 10006, "Param is invalid");
    return;
  }
  LOG(INFO) << "Config name from http: " << config_name;
  LOG(INFO) << "Config data from http: " << config_data;

  folly::dynamic null;
  std::unordered_map<std::string, std::string> configs;
  configs.insert(std::make_pair(config_name, config_data));
  database_manager_->updateConfigManually(configs);
  resultJson(response, null);
}

void HttpService::resultJson(service_framework::http::ServerResponse* response, const folly::dynamic& data) {
  sendResponse(response, 0, "", data);
}

void HttpService::errorJson(service_framework::http::ServerResponse* response, uint32_t code,
                            const std::string& error) {
  folly::dynamic data = folly::dynamic::object;
  sendResponse(response, code, error, data);
}

void HttpService::sendResponse(service_framework::http::ServerResponse* response, uint32_t code,
                               const std::string& error, const folly::dynamic& data) {
  folly::dynamic result = folly::dynamic::object;
  result.insert("Code", code);
  result.insert("Error", error);
  result.insert("Data", data);

  std::string response_body;
  if (common::toJson(&response_body, result)) {
    response->status(200, "OK").body(response_body).send();
  } else {
    response->status(500, "Server error").body("result object to json fail.").send();
  }
  return;
}

}  // namespace laser
