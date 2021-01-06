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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#include <list>
#include "common/laser/status.h"

#include "laser_service.h"

namespace laser {

constexpr char SERVICE_NAME[] = "laser_service";
constexpr char TIME_CONSUMING[] = "metric_rpc_times";
constexpr int TIMER_BUCKET_SCALE = 1;
constexpr int TIMER_MIN = 0;
constexpr int TIMER_MAX = 1000;

LaserService::LaserService(std::shared_ptr<ConfigManager> config_manager,
                           std::shared_ptr<DatabaseManager> database_manager)
    : config_manager_(config_manager),
      database_manager_(database_manager),
      traffic_restriction_config_(std::make_shared<TableTrafficRestrictionMap>()),
      second_traffic_restriction_config_(nullptr) {
  folly::SingletonVault::singleton()->registrationComplete();
  auto metrics = metrics::Metrics::getInstance();
  laser_service_timers_ = metrics->buildTimers(SERVICE_NAME, TIME_CONSUMING, TIMER_BUCKET_SCALE, TIMER_MIN, TIMER_MAX);
  config_manager_->subscribeTrafficRestrictionConfig(
      std::bind(&LaserService::updateTrafficRestrictionConfig, this, std::placeholders::_1));
  auto got_config = config_manager_->getTrafficRestrictionConfig();
  if (got_config != nullptr) {
    second_traffic_restriction_config_ = traffic_restriction_config_;
    traffic_restriction_config_ = got_config;
  }
}

void LaserService::getDatabaseEngine(std::shared_ptr<RocksDbEngine>* db, const std::unique_ptr<LaserKey>& key,
                                     std::shared_ptr<LaserKeyFormat> format_key) {
  const std::string& database_name = key->get_database_name();
  const std::string& table_name = key->get_table_name();
  auto partition_id = PartitionManager::getPartitionId(config_manager_, database_name, table_name, *format_key);
  if (!partition_id) {
    throwLaserException(Status::SERVICE_NOT_EXISTS_PARTITION, "get partition id fail");
  }

  auto partition = std::make_shared<Partition>(database_name, table_name, partition_id.value());
  auto db_engine = database_manager_->getDatabaseHandler(partition);
  if (!db_engine) {
    throwLaserException(Status::SERVICE_NOT_EXISTS_PARTITION,
                        folly::to<std::string>("get partition db engine fail, partition:", partition->describe()));
  }

  auto engine = db_engine.value().lock();
  if (!engine) {
    throwLaserException(Status::SERVICE_NOT_EXISTS_PARTITION,
                        folly::to<std::string>("get partition db engine fail, engine object has free, partition:",
                                               partition->describe()));
  }

  *db = engine;
}

std::shared_ptr<LaserKeyFormat> LaserService::getFormatKey(const std::unique_ptr<LaserKey>& key) {
  const std::vector<std::string>& primary_keys = key->get_primary_keys();
  const std::vector<std::string>& column_keys = key->get_column_keys();
  return std::make_shared<LaserKeyFormat>(primary_keys, column_keys);
}

void LaserService::commonCallEngine(std::unique_ptr<LaserKey> key, LaserServiceCallbackFunc func,
                                    const std::string& command_name) {
  metrics::Timer metric_time(laser_service_timers_.get());
  auto table_hash = config_manager_->getTableSchemaHash(key->get_database_name(), key->get_table_name());
  auto table_restriction = traffic_restriction_config_->find(table_hash);
  if (table_restriction != traffic_restriction_config_->end()) {
    if (table_restriction->second->getDenyAll()) {
      throwLaserException(Status::RS_OPERATION_DENIED, "request is denied in commonCallEngine,");
    }
    auto command_restriction = table_restriction->second->getSingleOperationLimits().find(command_name);
    if (command_restriction != table_restriction->second->getSingleOperationLimits().end()) {
      uint32_t random_value = folly::Random::rand32(1, 101);
      if (random_value > command_restriction->second) {
        throwLaserException(Status::RS_TRAFFIC_RESTRICTION, "request is denied in commonCallEngine,");
      }
    } else {
      throwLaserException(Status::RS_OPERATION_DENIED, "request is denied in commonCallEngine,");
    }
  }
  std::shared_ptr<RocksDbEngine> engine;
  auto format_key = getFormatKey(key);
  getDatabaseEngine(&engine, key, format_key);
  func(engine, format_key);
}

void LaserService::dispatchRequest(const std::vector<LaserKey>& keys, LaserServiceMultiDispatch func,
                                   const std::string& command_name) {
  metrics::Timer metric_time(laser_service_timers_.get());
  // 多个 key 的情况，从第一个 key 里获取 DatabaseName 和 TableName
  bool need_restrict_by_kps = false;
  std::string database_name = "";
  std::string table_name = "";
  if (!keys.empty()) {
    database_name = keys.begin()->get_database_name();
    table_name = keys.begin()->get_table_name();
  }
  auto table_hash = config_manager_->getTableSchemaHash(database_name, table_name);
  auto table_restriction = traffic_restriction_config_->find(table_hash);
  if (table_restriction != traffic_restriction_config_->end()) {
    if (table_restriction->second->getDenyAll()) {
      throwLaserException(Status::RS_OPERATION_DENIED, "request is denied in dispatchRequest,");
    }
    auto command_restriction = table_restriction->second->getMultipleOperationLimits().find(command_name);
    if (command_restriction != table_restriction->second->getMultipleOperationLimits().end()) {
      if (laser::TrafficRestrictionType::QPS == command_restriction->second.getType()) {
        uint32_t random_value = folly::Random::rand32(1, 101);
        if (random_value > command_restriction->second.getLimit()) {
          throwLaserException(Status::RS_TRAFFIC_RESTRICTION, "request is denied in dispatchRequest,");
        }
      } else {
        need_restrict_by_kps = true;
      }
    } else {
      throwLaserException(Status::RS_OPERATION_DENIED, "request is denied in dispatchRequest,");
    }
  }

  std::vector<std::shared_ptr<RocksDbEngine>> engines;
  std::unordered_map<RocksDbEngine*, std::vector<DispatchRequestItem>> dispatch_keys;

  uint32_t index = 0;
  for (auto& key : keys) {
    std::unique_ptr<LaserKey> pkey = std::make_unique<LaserKey>(key);
    std::shared_ptr<RocksDbEngine> engine;
    auto format_key = getFormatKey(pkey);
    try {
      getDatabaseEngine(&engine, pkey, format_key);
      if (engine.get() == nullptr) {
        continue;
      }
      engines.push_back(engine);

      if (dispatch_keys.find(engine.get()) == dispatch_keys.end()) {
        dispatch_keys[engine.get()] = std::vector<DispatchRequestItem>({});
      }
      DispatchRequestItem item;
      if (need_restrict_by_kps) {
        uint32_t random_value = folly::Random::rand32(1, 101);
        auto command_restriction = table_restriction->second->getMultipleOperationLimits().find(command_name);
        if (random_value > command_restriction->second.getLimit()) {
          item.deny_by_traffic_restriction = true;
          item.index = index++;
          dispatch_keys[engine.get()].push_back(item);
          continue;
        }
      }
      item.key = format_key;
      item.index = index;
      dispatch_keys[engine.get()].push_back(item);
    } catch (const LaserException& ex) {
      // nothing
    }
    index++;
  }

  func(dispatch_keys);
}

void LaserService::updateTrafficRestrictionConfig(const TableTrafficRestrictionMap& traffic_restrictions) {
  auto temp_map = std::make_shared<TableTrafficRestrictionMap>();
  for (auto& iter : traffic_restrictions) {
    auto table_traffic_restriction = std::make_shared<TableTrafficRestrictionConfig>(*iter.second);
    temp_map->insert(std::make_pair(iter.first, table_traffic_restriction));
  }
  second_traffic_restriction_config_ = traffic_restriction_config_;
  traffic_restriction_config_ = temp_map;
}

void LaserService::delkey(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     Status status = engine->delkey(*format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "del key fail,");
                     }
                   },
                   "delkey");
}

void LaserService::expire(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t time) {
  commonCallEngine(std::move(key),
                   [this, &response, time](auto engine, auto format_key) {
                     Status status = engine->expire(*format_key, time);
                     if (status != Status::OK) {
                       throwLaserException(status, "set expire time fail,");
                     }
                   },
                   "expire");
}

void LaserService::expireAt(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t time_at) {
  commonCallEngine(std::move(key),
                   [this, &response, time_at](auto engine, auto format_key) {
                     Status status = engine->expireAt(*format_key, time_at);
                     if (status != Status::OK) {
                       throwLaserException(status, "set expire time fail,");
                     }
                   },
                   "expireAt");
}

void LaserService::ttl(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     int64_t ttl;
                     Status status = engine->ttl(&ttl, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get ttl fail,");
                     }
                     response.set_int_data(ttl);
                   },
                   "ttl");
}

void LaserService::get(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     laser::LaserValueRawString value;
                     Status status = engine->get(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get string value fail,");
                     }

                     response.set_string_data(value.getValue());
                   },
                   "get");
}

void LaserService::append(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) {
  commonCallEngine(std::move(key),
                   [this, &response, &value](auto engine, auto format_key) {
                     uint32_t length = 0;
                     Status status = engine->append(&length, *format_key, *value);
                     if (status != Status::OK) {
                       throwLaserException(status, "append string value fail,");
                     }

                     response.set_int_data(length);
                   },
                   "append");
}

void LaserService::sset(LaserResponse& response, std::unique_ptr<LaserKV> kv) {
  std::unique_ptr<LaserKey> key = std::make_unique<LaserKey>(kv->key);
  std::unique_ptr<LaserValue> value = std::make_unique<LaserValue>(kv->value);
  commonCallEngine(std::move(key),
                   [this, &response, &value](auto engine, auto format_key) {
                     if (LaserValue::Type::string_value != value->getType()) {
                       throwLaserException(Status::CLIENT_UNION_DATA_TYPE_INVALID, "set string value fail,");
                     }
                     Status status = engine->set(*format_key, value->get_string_value());
                     if (status != Status::OK) {
                       throwLaserException(status, "set string value fail,");
                     }
                     response.set_int_data(value->get_string_value().size());
                   },
                   "set");
}

void LaserService::setx(LaserResponse& response, std::unique_ptr<LaserKV> kv, std::unique_ptr<LaserSetOption> option) {
  std::unique_ptr<LaserKey> key = std::make_unique<LaserKey>(kv->key);
  std::unique_ptr<LaserValue> value = std::make_unique<LaserValue>(kv->value);
  commonCallEngine(std::move(key),
                   [this, &response, &value, &option](auto engine, auto format_key) {
                     if (LaserValue::Type::string_value != value->getType()) {
                       throwLaserException(Status::CLIENT_UNION_DATA_TYPE_INVALID, "set string value fail,");
                     }
                     RocksDbEngineSetOptions rocksdb_set_option;
                     rocksdb_set_option.not_exists = option->get_not_exists();
                     rocksdb_set_option.ttl = option->get_ttl();
                     Status status = engine->setx(*format_key, value->get_string_value(), rocksdb_set_option);
                     if (status != Status::OK) {
                       throwLaserException(status, "set string value fail,");
                     }
                     response.set_int_data(value->get_string_value().size());
                   },
                   "setx");
}

// 获取失败或者不存在 db 都返回 null
void LaserService::mget(LaserResponse& response, std::unique_ptr<LaserKeys> keys) {
  const std::vector<LaserKey>& vec_keys = keys->get_keys();

  dispatchRequest(vec_keys,
                  [this, &response, &vec_keys](auto dispatch_keys) {
                    std::vector<LaserValue> values;
                    for (size_t i = 0; i < vec_keys.size(); i++) {
                      LaserValue value;
                      value.set_null_value(true);
                      values.push_back(std::move(value));
                    }

                    for (auto task : dispatch_keys) {
                      for (auto& item_key : task.second) {
                        LaserValue value;
                        if (item_key.deny_by_traffic_restriction) {
                          value.set_null_value(true);
                        } else {
                          laser::LaserValueRawString value_str;
                          Status status = task.first->get(&value_str, *(item_key.key));
                          if (status != Status::OK) {
                            value.set_null_value(true);
                          } else {
                            value.set_string_value(value_str.getValue());
                          }
                        }
                        values[item_key.index] = std::move(value);
                      }
                    }
                    response.set_list_value_data(std::move(values));
                  },
                  "mget");
}

// 获取失败或者不存在 db 都返回 null
/*
  1. 检设get是否是部分成功
  2. 获取每个key对应的状态，并返回给上层
*/
void LaserService::mgetDetail(LaserResponse& response, std::unique_ptr<LaserKeys> keys) {
  const std::vector<LaserKey>& vec_keys = keys->get_keys();
  dispatchRequest(vec_keys,
                  [this, &response, &vec_keys](auto dispatch_keys) {
                    std::vector<LaserValue> values;
                    for (size_t i = 0; i < vec_keys.size(); i++) {
                      LaserValue value;
                      EntryValue entry_value;
                      entry_value.set_status(Status::UNKNOWN_ERROR);
                      value.set_entry_value(entry_value);
                      values.push_back(std::move(value));
                    }
                    for (auto task : dispatch_keys) {
                      for (auto& item_key : task.second) {
                        LaserValue value;
                        EntryValue entry_value;
                        Status status;
                        if (item_key.deny_by_traffic_restriction) {
                          status = Status::RS_TRAFFIC_RESTRICTION;
                        } else {
                          laser::LaserValueRawString value_str;
                          status = task.first->get(&value_str, *(item_key.key));
                          if (status == Status::OK) {
                            entry_value.set_string_value(value_str.getValue());
                          }
                        }
                        entry_value.set_status(status);
                        value.set_entry_value(entry_value);
                        values[item_key.index] = std::move(value);
                      }
                    }
                    response.set_list_value_data(std::move(values));
                  },
                  "mgetDetail");
}

void LaserService::mset(LaserResponse& response, std::unique_ptr<LaserKVs> values) {
  const std::vector<LaserKV>& kvs = values->get_values();
  std::vector<LaserKey> keys;
  std::vector<LaserValue> vec_values;
  for (auto& kv : kvs) {
    keys.push_back(kv.get_key());
    vec_values.push_back(kv.get_value());
  }

  dispatchRequest(keys,
                  [this, &response, &vec_values](auto dispatch_keys) {
                    std::vector<int64_t> result(vec_values.size(), -1);
                    for (auto task : dispatch_keys) {
                      std::vector<LaserKeyFormat> batch_keys;
                      std::vector<std::string> data;
                      std::vector<uint32_t> pass_indexes;
                      for (auto& item_key : task.second) {
                        if (item_key.deny_by_traffic_restriction) {
                          result[item_key.index] = -1;
                        } else {
                          data.push_back(vec_values[item_key.index].get_string_value());
                          batch_keys.push_back(*(item_key.key));
                          pass_indexes.push_back(item_key.index);
                        }
                      }
                      Status status = task.first->mset(batch_keys, data);
                      for (auto& index : pass_indexes) {
                        if (status != Status::OK) {
                          result[index] = -1;
                        } else {
                          result[index] = index;
                        }
                      }
                    }
                    response.set_list_int_data(std::move(result));
                  },
                  "mset");
}

void LaserService::msetDetail(LaserResponse& response, std::unique_ptr<LaserKVs> values,
                              std::unique_ptr<LaserSetOption> option) {
  const std::vector<LaserKV>& kvs = values->get_values();
  std::vector<LaserKey> keys;
  std::vector<LaserValue> vec_values;
  for (auto& kv : kvs) {
    keys.push_back(kv.get_key());
    vec_values.push_back(kv.get_value());
  }

  dispatchRequest(keys,
                  [this, &response, &vec_values, &option](auto dispatch_keys) {
                    RocksDbEngineSetOptions rocksdb_set_option;
                    rocksdb_set_option.not_exists = option->get_not_exists();
                    rocksdb_set_option.ttl = option->get_ttl();
                    std::vector<LaserValue> values;
                    for (size_t i = 0; i < vec_values.size(); i++) {
                      LaserValue value;
                      EntryValue entry_value;
                      entry_value.set_status(Status::UNKNOWN_ERROR);
                      value.set_entry_value(entry_value);
                      values.push_back(std::move(value));
                    }
                    for (auto task : dispatch_keys) {
                      std::vector<LaserKeyFormat> batch_keys;
                      std::vector<std::string> data;
                      std::vector<uint32_t> pass_indexes;
                      for (auto& item_key : task.second) {
                        if (item_key.deny_by_traffic_restriction) {
                          LaserValue value;
                          EntryValue entry_value;
                          entry_value.set_status(Status::RS_TRAFFIC_RESTRICTION);
                          value.set_entry_value(entry_value);
                          values[item_key.index] = std::move(value);
                        } else {
                          data.push_back(vec_values[item_key.index].get_string_value());
                          batch_keys.push_back(*(item_key.key));
                          pass_indexes.push_back(item_key.index);
                        }
                      }
                      Status status = task.first->msetx(batch_keys, data, rocksdb_set_option);
                      for (auto index : pass_indexes) {
                        LaserValue value;
                        EntryValue entry_value;
                        entry_value.set_status(status);
                        value.set_entry_value(entry_value);
                        values[index] = std::move(value);
                      }
                    }
                    response.set_list_value_data(std::move(values));
                  },
                  "msetDetail");
}

void LaserService::mdel(LaserResponse& response, std::unique_ptr<LaserKeys> keys) {
  const std::vector<LaserKey>& vec_keys = keys->get_keys();
  dispatchRequest(vec_keys,
                  [this, &response, &vec_keys](auto dispatch_keys) {
                    std::vector<LaserValue> values;
                    for (size_t i = 0; i < vec_keys.size(); i++) {
                      LaserValue value;
                      EntryValue entry_value;
                      entry_value.set_status(Status::UNKNOWN_ERROR);
                      value.set_entry_value(entry_value);
                      values.push_back(std::move(value));
                    }
                    for (auto task : dispatch_keys) {
                      for (auto& item_key : task.second) {
                        LaserValue value;
                        EntryValue entry_value;
                        Status status;
                        if (item_key.deny_by_traffic_restriction) {
                          status = Status::RS_TRAFFIC_RESTRICTION;
                        } else {
                          status = task.first->delkey(*(item_key.key));
                        }
                        entry_value.set_status(status);
                        value.set_entry_value(entry_value);
                        values[item_key.index] = std::move(value);
                      }
                    }
                    response.set_list_value_data(std::move(values));
                  },
                  "mdel");
}

void LaserService::exist(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     bool value;
                     Status status = engine->exist(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get exist value is  fail,");
                     }

                     response.set_bool_data(value);
                   },
                   "exist");
}

void LaserService::hgetall(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(
      std::move(key),
      [this, &response](auto engine, auto format_key) {
        std::unordered_map<std::string, LaserValueRawString> values;
        Status status = engine->hgetall(&values, *format_key);
        if (status != Status::OK) {
          throwLaserException(status, "get all hash value fail,");
        }
        std::map<std::string, std::string> map_string_data;
        for (auto& value : values) {
          map_string_data.insert(std::pair<std::string, std::string>(value.first, value.second.getValue()));
        }
        response.set_map_string_data(map_string_data);
      },
      "hgetall");
}

void LaserService::hmget(LaserResponse& response, std::unique_ptr<LaserKey> key,
                         std::unique_ptr<std::vector<std::string>> fields) {
  commonCallEngine(std::move(key),
                   [this, &response, &fields](auto engine, auto format_key) {
                     std::unordered_map<std::string, LaserValueRawString> values;
                     Status status = engine->hgetall(&values, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hmget all hash value fail,");
                     }

                     std::map<std::string, std::string> map_string_data;
                     for (auto& field : *fields) {
                       if (values.find(field) != values.end()) {
                         map_string_data.insert(std::pair<std::string, std::string>(field, values[field].getValue()));
                       }
                     }
                     response.set_map_string_data(map_string_data);
                   },
                   "hmget");
}

void LaserService::hget(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) {
  commonCallEngine(std::move(key),
                   [this, &response, &field](auto engine, auto format_key) {
                     laser::LaserValueRawString value;

                     Status status = engine->hget(&value, *format_key, *field);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hget hash value fail,");
                     }
                     response.set_string_data(value.getValue());
                   },
                   "hget");
}

void LaserService::hlen(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     LaserValueMapMeta value;
                     Status status = engine->hlen(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hlen hash value fail,");
                     }
                     response.set_int_data(value.getSize());
                   },
                   "hlen");
}

void LaserService::hexists(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) {
  commonCallEngine(std::move(key),
                   [this, &response, &field](auto engine, auto format_key) {
                     laser::LaserValueRawString value;

                     Status status = engine->hget(&value, *format_key, *field);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hexists value fail,");
                     }
                     response.set_int_data(1);
                   },
                   "hexists");
}

void LaserService::hkeys(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     std::vector<LaserKeyFormatMapData> values;

                     Status status = engine->hkeys(&values, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hkeys value fail,");
                     }
                     std::vector<std::string> data_list;
                     for (auto value : values) {
                       data_list.push_back(value.getField());
                     }
                     response.set_list_string_data(data_list);
                   },
                   "hkeys");
}

void LaserService::hdel(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field) {
  commonCallEngine(std::move(key),
                   [this, &response, &field](auto engine, auto format_key) {
                     Status status = engine->hdel(*format_key, *field);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hdel hash value fail,");
                     }
                     response.set_int_data(1);
                   },
                   "hdel");
}

void LaserService::hset(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> field,
                        std::unique_ptr<std::string> value) {
  commonCallEngine(std::move(key),
                   [this, &response, &field, &value](auto engine, auto format_key) {
                     Status status = engine->hset(*format_key, *field, *value);
                     if (status != Status::OK) {
                       throwLaserException(status, "get hset hash value fail,");
                     }
                     response.set_int_data(1);
                   },
                   "hset");
}

void LaserService::hmset(LaserResponse& response, std::unique_ptr<LaserKey> key, std::unique_ptr<LaserValue> values) {
  commonCallEngine(std::move(key),
                   [this, &response, &values](auto engine, auto format_key) {
                     if (LaserValue::Type::map_value != values->getType()) {
                       throwLaserException(Status::CLIENT_UNION_DATA_TYPE_INVALID, "hmset value type is invalid,");
                     }

                     Status status = engine->hmset(*format_key, values->get_map_value());
                     if (status != Status::OK) {
                       throwLaserException(status, "get hmset value fail,");
                     }
                     response.set_int_data(values->get_map_value().size());
                   },
                   "hmset");
}

void LaserService::lindex(LaserResponse& response, std::unique_ptr<LaserKey> key, int32_t index) {
  commonCallEngine(std::move(key),
                   [this, &response, index](auto engine, auto format_key) {
                     LaserValueRawString value;
                     Status status = engine->lindex(&value, *format_key, index);
                     if (status != Status::OK) {
                       throwLaserException(status, "lindex value fail,");
                     }
                     response.set_string_data(value.getValue());
                   },
                   "lindex");
}

void LaserService::llen(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     LaserValueListMeta value;
                     Status status = engine->llen(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "llen value fail,");
                     }
                     response.set_int_data(value.getSize());
                   },
                   "llen");
}

void LaserService::lpop(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     LaserValueRawString value;
                     Status status = engine->popFront(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "lpop value fail,");
                     }
                     response.set_string_data(value.getValue());
                   },
                   "lpop");
}

void LaserService::lpush(LaserResponse&, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) {
  commonCallEngine(std::move(key),
                   [this, &value](auto engine, auto format_key) {
                     Status status = engine->pushFront(*format_key, *value);
                     if (status != Status::OK) {
                       throwLaserException(status, "lpush value fail,");
                     }
                   },
                   "lpush");
}

void LaserService::rpop(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     LaserValueRawString value;
                     Status status = engine->popBack(&value, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "rpop value fail,");
                     }
                     response.set_string_data(value.getValue());
                   },
                   "rpop");
}

void LaserService::rpush(LaserResponse&, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> value) {
  commonCallEngine(std::move(key),
                   [this, &value](auto engine, auto format_key) {
                     Status status = engine->pushBack(*format_key, *value);
                     if (status != Status::OK) {
                       throwLaserException(status, "rpush value fail,");
                     }
                   },
                   "rpush");
}

void LaserService::lrange(LaserResponse& response, std::unique_ptr<LaserKey> key, int32_t start, int32_t end) {
  commonCallEngine(std::move(key),
                   [this, &response, start, end](auto engine, auto format_key) {
                     std::vector<std::string> data_list;
                     std::vector<LaserValueRawString> value;
                     Status status = engine->lrange(&value, *format_key, start, end);
                     if (status != Status::OK) {
                       throwLaserException(status, "lrange value fail,");
                     }
                     for (auto val : value) {
                       data_list.push_back(val.getValue());
                     }
                     response.set_list_string_data(data_list);
                   },
                   "lrange");
}

void LaserService::decrBy(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t step) {
  commonCallEngine(std::move(key),
                   [this, &response, step](auto engine, auto format_key) {
                     int64_t value;
                     Status status = engine->decr(&value, *format_key, step);
                     if (status != Status::OK) {
                       throwLaserException(status, "decrBy fail,");
                     }
                     response.set_int_data(value);
                   },
                   "decrBy");
}

void LaserService::incrBy(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t step) {
  commonCallEngine(std::move(key),
                   [this, &response, step](auto engine, auto format_key) {
                     int64_t value;
                     Status status = engine->incr(&value, *format_key, step);
                     if (status != Status::OK) {
                       throwLaserException(status, "incrBy fail,");
                     }
                     response.set_int_data(value);
                   },
                   "incrBy");
}

void LaserService::decr(LaserResponse& response, std::unique_ptr<LaserKey> key) { decrBy(response, std::move(key), 1); }

void LaserService::incr(LaserResponse& response, std::unique_ptr<LaserKey> key) { incrBy(response, std::move(key), 1); }

void LaserService::sadd(LaserResponse&, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) {
  commonCallEngine(std::move(key),
                   [this, &member](auto engine, auto format_key) {
                     Status status = engine->sadd(*format_key, *member);
                     if (status != Status::OK) {
                       throwLaserException(status, "sadd fail,");
                     }
                   },
                   "sadd");
}

void LaserService::sismember(LaserResponse&, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) {
  commonCallEngine(std::move(key),
                   [this, &member](auto engine, auto format_key) {
                     Status status = engine->hasMember(*format_key, *member);
                     if (status != Status::OK) {
                       throwLaserException(status, "sismember fail,");
                     }
                   },
                   "sismember");
}

void LaserService::sremove(LaserResponse&, std::unique_ptr<LaserKey> key, std::unique_ptr<std::string> member) {
  commonCallEngine(std::move(key),
                   [this, &member](auto engine, auto format_key) {
                     Status status = engine->sdel(*format_key, *member);
                     if (status != Status::OK) {
                       throwLaserException(status, "sdel fail,");
                     }
                   },
                   "sremove");
}

void LaserService::smembers(LaserResponse& response, std::unique_ptr<LaserKey> key) {
  commonCallEngine(std::move(key),
                   [this, &response](auto engine, auto format_key) {
                     std::vector<std::string> data_list;
                     std::vector<LaserKeyFormatSetData> values;
                     Status status = engine->members(&values, *format_key);
                     if (status != Status::OK) {
                       throwLaserException(status, "smembers fail,");
                     }
                     for (auto val : values) {
                       data_list.push_back(val.getData());
                     }
                     response.set_list_string_data(data_list);
                   },
                   "smembers");
}

void LaserService::zadd(LaserResponse& response, std::unique_ptr<LaserKey> key,
                        std::unique_ptr<LaserValue> member_scores) {
  commonCallEngine(std::move(key),
                   [this, &response, &member_scores](auto engine, auto format_key) {
                     int64_t number = 0;
                     Status status = engine->zadd(*format_key, member_scores->get_member_score_value());
                     if (status != Status::OK) {
                       throwLaserException(status, "zadd fail,");
                     }
                     // engine执行zadd成功，默认所有member全部第一次添加
                     number = member_scores->get_member_score_value().size();
                     response.set_int_data(number);
                   },
                   "zadd");
}

void LaserService::zrangeByScore(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t min, int64_t max) {
  commonCallEngine(std::move(key),
                   [this, &response, &min, &max](auto engine, auto format_key) {
                     std::vector<LaserScoreMember> score_member_data_list;
                     Status status = engine->zrangeByScore(&score_member_data_list, *format_key, min, max);
                     if (status != Status::OK) {
                       throwLaserException(status, "zrangeByScore fail,");
                     }
                     response.set_list_score_member_data(score_member_data_list);
                   },
                   "zrangeByScore");
}

void LaserService::zremRangeByScore(LaserResponse& response, std::unique_ptr<LaserKey> key, int64_t min, int64_t max) {
  commonCallEngine(std::move(key),
                   [this, &response, min, max](auto engine, auto format_key) {
                     int64_t number = 0;
                     Status status = engine->zremRangeByScore(&number, *format_key, min, max);
                     if (status != Status::OK) {
                       throwLaserException(status, "zremRangeByScore fail,");
                     }
                     response.set_int_data(number);
                   },
                   "zremRangeByScore");
}

}  // namespace laser
