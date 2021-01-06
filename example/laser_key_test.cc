#include <fstream>

#include "folly/init/Init.h"
#include "folly/Random.h"

#include "common/service_router/http.h"
#include "common/laser/status.h"
#include "client/laser_client.h"
#include "common/metrics/metrics.h"

DEFINE_string(host, "127.0.0.1", "current service host address");
DEFINE_string(local_host, "127.0.0.1", "current service host address");
DEFINE_int32(port, 0, "current service port");
DEFINE_string(service_name, "laser_client", "Current geo client service name");
DEFINE_string(target_service_name, "laser_dev", "Search laser service name");
DEFINE_string(database_name, "test", "Test laser database name");
DEFINE_string(table_names, "test_raw_string", "Test laser table names");
DEFINE_string(command, "get", "Test laser command");
DEFINE_int32(numbers, 10000, "Send laser server numbers.");
DEFINE_int32(batch_number, 10, "Send laser batch multi request numbers.");
DEFINE_int32(num_clients, 16, "Number of clients to use. (Default: 1 per core)");
DEFINE_bool(print, false, "Print result to stdout");
DEFINE_int32(value_size, 128, "Value size");

DEFINE_int32(rpc_request_timeout, 10, "each request recv timeout");
DEFINE_int32(diff_range, 256, "Address diff range");
DEFINE_string(load_balance_method, "random",
              "request load balance method `random/roundrobin/localfirst/configurable_weight`");
DEFINE_string(client_request_read_mode, "mixed_read", "request read mode `leader_read/mixed_read");
DEFINE_int32(max_conn_per_server, 0, "Max connection pre server.");
DEFINE_int32(thrift_compression_method, 0, "thrift compression method");

DEFINE_int32(max_key_id, 10, "Max key id");
DEFINE_int32(max_key_bucket, 32, "Max key create bucket");

DEFINE_int32(hash_field_size, 10, "hash value field size");
DEFINE_int32(zadd_data_num, 10, "zadd add 10 data to db");
DEFINE_int32(zrangebyscore_data_num, 5, "zrangebyscore show 5 data here");
DEFINE_int32(zremrangebyscore_data_num, 8, "zremrangebyscore remove 8 data here");

constexpr static char KEY_BUCKET_AND_ID_SPLIT[] = "|";
constexpr char LASER_CLIENT_MODULE_NAME[] = "laser_client";
constexpr char LASER_CLIENT_TABLE_CALL[] = "table_call";
constexpr double LASER_CLIENT_CALL_METRIC_CALL_BUCKET_SIZE = 1.0;
constexpr double LASER_CLIENT_CALL_METRIC_CALL_MIN = 0.0;
constexpr double LASER_CLIENT_CALL_METRIC_CALL_MAX = 1000.0;

class LaserCall {
 public:
  LaserCall(const std::string& target_service_name, const std::string& database_name, const std::string& table_names)
      : target_service_name_(target_service_name), database_name_(database_name), table_names_(table_names) {}
  ~LaserCall() = default;
  void init() {
    client_ = std::make_shared<laser::LaserClient>(target_service_name_);
    client_->init();
    client_options_ = getClientOption();
  }

  const laser::ClientOption getClientOption() {
    laser::ClientOption option;
    option.setReceiveTimeoutMs(FLAGS_rpc_request_timeout);
    option.setMaxConnPerServer(FLAGS_max_conn_per_server);
    option.setThriftCompressionMethod(FLAGS_thrift_compression_method);
    auto method = service_router::stringToLoadBalanceMethod(FLAGS_load_balance_method);
    service_router::LoadBalanceMethod load_method = service_router::LoadBalanceMethod::RANDOM;
    if (!method) {
      FB_LOG_EVERY_MS(ERROR, 1000) << "Specified load balance method is invalid, default is random";
    } else {
      load_method = *method;
    }
    option.setLoadBalance(load_method);

    auto read_mode = laser::ClientRequestReadMode::MIXED_READ;
    auto read_mode_optional = laser::stringToClientRequestReadMode(FLAGS_client_request_read_mode);
    if (read_mode_optional) {
      read_mode = *read_mode_optional;
    }
    option.setReadMode(read_mode);
    service_router::BalanceLocalFirstConfig local_first;
    local_first.setLocalIp(FLAGS_local_host);
    local_first.setDiffRange(FLAGS_diff_range);
    option.setLocalFirstConfig(local_first);
    return option;
  }

  void run(bool is_print, uint32_t numbers, uint32_t thread_nums) {
    std::vector<std::string> tables;
    folly::split(',', table_names_, tables);
    if (tables.empty()) {
      LOG(INFO) << "Stress test table is empty.";
      return;
    }
    for (auto& table_name : tables) {
      std::unordered_map<std::string, std::string> tags = {{"table_name", table_name}};
      timers_[table_name] = metrics::Metrics::getInstance()->buildTimers(
          LASER_CLIENT_MODULE_NAME, LASER_CLIENT_TABLE_CALL, LASER_CLIENT_CALL_METRIC_CALL_BUCKET_SIZE,
          LASER_CLIENT_CALL_METRIC_CALL_MIN, LASER_CLIENT_CALL_METRIC_CALL_MAX, tags);
    }
    while (is_running_) {
      for (auto& table_name : tables) {
        std::vector<std::unique_ptr<std::thread>> threads;
        for (int i = 0; i < thread_nums; ++i) {
          threads.push_back(std::make_unique<std::thread>([this, is_print, table_name, numbers]() {
            for (uint32_t i = 0; i < numbers; i++) {
              call(table_name, is_print);
            }
          }));
        }

        for (auto& thr : threads) {
          thr->join();
        }
      }
    }
  }

  void stop() { is_running_ = false; }

 private:
  std::string target_service_name_;
  std::string database_name_;
  std::string table_names_;
  std::shared_ptr<laser::LaserClient> client_;
  std::atomic<bool> is_running_{true};
  laser::ClientOption client_options_;
  std::atomic<uint32_t> current_bucket_id_{0};
  std::atomic<uint32_t> key_id_{0};
  std::unordered_map<std::string, std::shared_ptr<metrics::Timers>> timers_;

  void call(const std::string& table_name, bool is_print) {
    metrics::Timer timer(timers_[table_name].get());

    if (FLAGS_command == "mget") {
      std::vector<laser::LaserKey> keys;
      getLaserKeys(&keys, table_name);
      mget(keys, table_name, is_print);
    } else if (FLAGS_command == "mgetDetail") {
      std::vector<laser::LaserKey> keys;
      getLaserKeys(&keys, table_name);
      mgetDetail(keys, table_name, is_print);
    } else if (FLAGS_command == "mdel") {
      std::vector<laser::LaserKey> keys;
      getLaserKeys(&keys, table_name);
      mdel(keys, table_name, is_print);
    } else if (FLAGS_command == "setget") {
      setget(table_name, is_print);
    } else if (FLAGS_command == "exist") {
      exist(table_name, is_print);
    } else if (FLAGS_command == "msetget") {
      msetget(table_name, is_print);
    } else if (FLAGS_command == "mset") {
      mset(table_name, is_print);
    } else if (FLAGS_command == "msetDetailget") {
      msetDetailget(table_name, is_print);
    } else if (FLAGS_command == "hmsetget") {
      hmset(table_name, is_print, true);
    } else if (FLAGS_command == "hmset") {
      hmset(table_name, is_print, false);
    } else if (FLAGS_command == "hgetall") {
      laser::LaserKey key;
      getLaserKey(&key, table_name);
      hgetall(key, is_print);
    } else if (FLAGS_command == "zadd") {
      zadd(table_name, is_print);
    } else if (FLAGS_command == "zrangebyscore") {
      zrangebyscore(table_name, is_print);
    } else if (FLAGS_command == "zremrangebyscore") {
      zremrangebyscore(table_name, is_print);
    } else if (FLAGS_command == "set") {
      set(table_name, is_print);
    } else {
      laser::LaserKey key;
      getLaserKey(&key, table_name);
      get(key, table_name, is_print);
    }
  }

  void get(const laser::LaserKey& key, const std::string& table_name, bool is_print) {
    std::string data;
    auto ret = client_->getSync(client_options_, &data, key);
    if (!is_print) {
      return;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call get api fail," << ret;
    } else {
      LOG(INFO) << "pk:" << key.get_primary_keys()[0] << " vlaue:" << data;
    }
  }

  void setget(const std::string& table_name, bool is_print) {
    laser::LaserKV kv;
    getLaserKV(&kv, table_name);

    auto ret = client_->setSync(client_options_, kv);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call setget api fail," << ret;
    }
    const laser::LaserKey key = kv.get_key();
    get(kv.get_key(), table_name, is_print);
  }

  void set(const std::string& table_name, bool is_print) {
    laser::LaserKV kv;
    getLaserKV(&kv, table_name);

    auto ret = client_->setSync(client_options_, kv);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call set api fail," << ret;
    }
  }

  void exist(const std::string& table_name, bool is_print) {
    laser::LaserKV kv;
    getLaserKV(&kv, table_name);
    const laser::LaserKey key = kv.get_key();

    bool data;
    auto ret = client_->existSync(client_options_, &data, key);
    if (!is_print) {
      return;
    }
    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call exist api fail," << ret;
    } else {
      LOG(INFO) << "Exist command pk is:" << key.get_primary_keys()[0] << " exist:" << data;
    }
  }

  void msetget(const std::string& table_name, bool is_print) {
    std::vector<laser::LaserKV> kvs;
    getLaserKVs(&kvs, table_name);
    std::vector<int64_t> result;

    auto ret = client_->mset(client_options_, &result, kvs);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call msetget api fail," << ret;
    }
    std::vector<laser::LaserKey> keys;
    for (auto& kv : kvs) {
      keys.push_back(kv.get_key());
    }
    mget(keys, table_name, is_print);
  }

  void mset(const std::string& table_name, bool is_print) {
    std::vector<laser::LaserKV> kvs;
    getLaserKVs(&kvs, table_name);
    std::vector<int64_t> result;

    auto ret = client_->mset(client_options_, &result, kvs);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call msetget api fail," << ret;
    }
  }

  void msetDetailget(const std::string& table_name, bool is_print) {
    std::vector<laser::LaserKV> kvs;
    getLaserKVs(&kvs, table_name);
    std::vector<laser::LaserValue> result;

    auto ret = client_->msetDetail(client_options_, &result, kvs);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call msetDetailget api fail," << ret;
    }
    std::vector<laser::LaserKey> keys;
    for (auto& kv : kvs) {
      keys.push_back(kv.get_key());
    }

    std::vector<laser::LaserValue> values;
    ret = client_->mgetDetail(client_options_, &values, keys);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call mgetDetail api return not ok." << ret;
    }
  }

  void mget(const std::vector<laser::LaserKey>& keys, const std::string& table_name, bool is_print) {
    std::vector<laser::LaserValue> values;
    auto ret = client_->mget(client_options_, &values, keys);
    if (!is_print) {
      return;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call get api fail," << ret;
      return;
    }

    LOG(INFO) << "Start print result:";
    for (uint32_t i = 0; i < values.size(); i++) {
      auto& value = values[i];
      auto value_type = value.getType();
      auto& pk = keys[i].get_primary_keys();
      if (laser::LaserValue::Type::string_value == value_type) {
        LOG(INFO) << "key:" << pk[0] << "value:" << value.get_string_value();
      } else if (laser::LaserValue::Type::null_value == value_type) {
        LOG(INFO) << "key:" << pk[0] << "value: is null";
      } else {
        LOG(INFO) << "value: is invalid";
      }
    }
  }

  void setData(const std::string& table_name, bool is_print) {
    std::vector<laser::LaserKV> kvs;
    getLaserKVs(&kvs, table_name);
    for (auto& kv : kvs) {
      auto ret = client_->setSync(client_options_, kv);
      if (is_print) {
        if (ret != laser::Status::OK) {
          LOG(INFO) << "Call set api fail," << ret;
        }
      }
    }
  }

  void mgetDetail(const std::vector<laser::LaserKey>& keys, const std::string& table_name, bool is_print) {
    std::vector<laser::LaserValue> values;
    // 调用set插入数据，再通过mgetDetail获取返回信息
    setData(table_name, is_print);
    // 通过mgetDetail获取返回信息
    auto ret = client_->mgetDetail(client_options_, &values, keys);
    if (!is_print) {
      return;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call mgetDetail api return not ok." << ret;
    }

    LOG(INFO) << "Start print result:";
    for (uint32_t i = 0; i < values.size(); i++) {
      auto& pk = keys[i].get_primary_keys();
      LOG(ERROR) << "key:" << pk[0] << ", res:" << values[i].get_entry_value().get_status();
      LOG(ERROR) << "key:" << pk[0] << ", value:" << values[i].get_entry_value().get_string_value();
    }
  }
  void mdel(const std::vector<laser::LaserKey>& keys, const std::string& table_name, bool is_print) {
    std::vector<laser::LaserValue> values;
    // 调用set插入数据，再通过mdel删除数据
    setData(table_name, is_print);
    // 通过mdel删除key
    auto ret = client_->mdel(client_options_, &values, keys);
    if (!is_print) {
      return;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call mdel api return not ok." << ret;
    }

    LOG(INFO) << "Start print result:";
    for (uint32_t i = 0; i < values.size(); i++) {
      auto& pk = keys[i].get_primary_keys();
      LOG(ERROR) << "mdel : key:" << pk[0] << ", res:" << values[i].get_entry_value().get_status();
    }
  }

  void hmset(const std::string& table_name, bool is_print, bool isget = false) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, table_name);
    auto pks = laser_key.get_primary_keys();
    if (pks.empty()) {
      LOG(INFO) << "laser key: is invalid";
      return;
    }

    laser::LaserValue values;
    std::map<std::string, std::string> values_map;
    for (uint32_t i = 0; i < FLAGS_hash_field_size; i++) {
      values_map[folly::to<std::string>(pks[0], i)] = pks[0];
    }
    values.set_map_value(values_map);
    auto ret = client_->hmsetSync(client_options_, laser_key, values);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call hmset api fail," << ret;
      return;
    }

    if (!isget) {
      return;
    }

    hgetall(laser_key, is_print);
  }

  void hgetall(const laser::LaserKey& laser_key, bool is_print) {
    std::map<std::string, std::string> data;
    auto ret = client_->hgetallSync(client_options_, &data, laser_key);
    if (!is_print) {
      return;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call get api fail," << ret;
      return;
    }

    LOG(INFO) << "Start print result:";
    auto& pks = laser_key.get_primary_keys();
    for (auto& value : data) {
      LOG(INFO) << "key:" << pks[0] << " field:" << value.first << " value:" << value.second;
    }
  }

  void getLaserKeys(std::vector<laser::LaserKey>* keys, const std::string& table_name) {
    for (int i = 0; i < FLAGS_batch_number; i++) {
      laser::LaserKey laser_key;
      getLaserKey(&laser_key, table_name);
      keys->push_back(std::move(laser_key));
    }
  }

  void getLaserKey(laser::LaserKey* laser_key, const std::string& table_name) {
    uint32_t key_id = key_id_.fetch_add(1);
    if (key_id_ > FLAGS_max_key_id) {
      key_id_.store(0);
    }
    uint32_t bucket_id = current_bucket_id_.fetch_add(1);
    if (current_bucket_id_ > FLAGS_max_key_bucket) {
      current_bucket_id_.store(0);
    }
    std::string pk = folly::to<std::string>(getKey(bucket_id, key_id));

    std::vector<std::string> pks({pk});
    laser_key->set_database_name(database_name_);
    laser_key->set_table_name(table_name);
    laser_key->set_primary_keys(pks);
  }

  void getLaserKVs(std::vector<laser::LaserKV>* kvs, const std::string& table_name) {
    for (int i = 0; i < FLAGS_batch_number; i++) {
      laser::LaserKV kv;
      getLaserKV(&kv, table_name);
      kvs->push_back(std::move(kv));
    }
  }

  void getLaserKV(laser::LaserKV* kv, const std::string& table_name) {
    laser::LaserKey key;
    getLaserKey(&key, table_name);
    laser::LaserValue value;

    std::string value_str = paddingValue(key.get_primary_keys()[0]);
    size_t repeat_step = std::ceil(std::log2(FLAGS_value_size) - std::log2(value_str.size()));
    for (int i = 0; i < repeat_step; i++) {
      value_str += value_str;
    }
    value.set_string_value(value_str);
    kv->set_key(key);
    kv->set_value(value);
  }

  std::string paddingValue(const std::string& value) {
    std::string result = value;
    size_t new_len = std::pow(2, std::ceil(std::log2(value.size())));
    for (size_t i = 0; i < (new_len - value.size()); i++) {
      result.append(1, '0');
    }

    return result;
  }

  int64_t getKey(size_t bucket_id, size_t key_id) {
    int64_t key = CityHash64WithSeed(KEY_BUCKET_AND_ID_SPLIT, 1, bucket_id);
    std::string key_id_str = folly::to<std::string>(key_id);
    key = CityHash64WithSeed(key_id_str.c_str(), key_id_str.size(), key);
    return key;
  }

  void getMemberScores(std::unordered_map<std::string, double>* member_scores) {
    for (uint32_t i = 0; i < FLAGS_zadd_data_num; i++) {
      std::string str_tmp;
      str_tmp = "member_" + std::to_string(i);
      (*member_scores)[str_tmp] = i;
    }
  }

  bool zaddData(const laser::LaserKey& laser_key, bool is_print) {
    std::unordered_map<std::string, double> map_member_scores;
    getMemberScores(&map_member_scores);

    uint32_t res;
    auto ret = client_->zaddSync(client_options_, &res, laser_key, map_member_scores);
    if ((ret != laser::Status::OK) && (is_print)) {
      LOG(INFO) << "Call zadd api fail," << ret;
      return false;
    }
    return true;
  }

  void zadd(const std::string& table_name, bool is_print) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, table_name);
    uint32_t res;
    std::unordered_map<std::string, double> member_scores;
    getMemberScores(&member_scores);

    auto ret = client_->zaddSync(client_options_, &res, laser_key, member_scores);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call zadd api fail," << ret;
      return;
    }
  }

  void zrangebyscore(const std::string& table_name, bool is_print) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, table_name);

    // 先调用zaddSync插入数据，再通过zrangebyscore获取返回值
    auto is_sucess = zaddData(laser_key, is_print);
    if (!is_sucess) {
      return;
    }

    int64_t num_min = 1;
    int64_t num_max = FLAGS_zrangebyscore_data_num;
    std::vector<laser::LaserFloatScoreMember> member_scores;
    auto ret = client_->zrangebyscoreSync(client_options_, &member_scores, laser_key, num_min, num_max);
    if (is_print && ret != laser::Status::OK) {
      LOG(INFO) << "Call zrangebyscoreSync api fail." << ret;
      return;
    } else {
      for (auto member_score : member_scores) {
        LOG(INFO) << "member : " << member_score.getMember();
        LOG(INFO) << "score : " << member_score.getScore();
      }
    }
  }

  void zremrangebyscore(const std::string& table_name, bool is_print) {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, table_name);

    // 先调用zaddSync插入数据，再通过zremrangebyscore删除
    auto is_sucess = zaddData(laser_key, is_print);
    if (!is_sucess) {
      return;
    }

    int64_t num_min = 1;
    int64_t num_max = FLAGS_zremrangebyscore_data_num;
    uint32_t number = 0;
    auto ret2 = client_->zremrangebyscoreSync(client_options_, &number, laser_key, num_min, num_max);
    if (is_print && ret2 != laser::Status::OK) {
      LOG(INFO) << "Call zremrangebyscoreSync api fail." << ret2;
      return;
    } else {
      LOG(INFO) << "number : " << number;
    }
  }
};

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = true;
  folly::Init init(&argc, &argv);

  SCOPE_EXIT {
    service_router::unregisterAll();
    service_framework::http::stop();
    service_router::stop_connection_pool();
  };

  LaserCall laser_call(FLAGS_target_service_name, FLAGS_database_name, FLAGS_table_names);
  laser_call.init();

  std::thread http_server_thread(
      []() { service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_port); });

  laser_call.run(FLAGS_print, FLAGS_numbers, FLAGS_num_clients);
  http_server_thread.join();
  return 0;
}
