#include <fstream>
#include "folly/init/Init.h"
#include "folly/Random.h"

#include "service_router/http.h"
#include "laser/lib/loader_source_data.h"
#include "laser/lib/status.h"
#include "laser/client/laser_client.h"
#include "common/metrics/metrics.h"

DEFINE_string(host, "127.0.0.1", "current service host address");
DEFINE_int32(port, 0, "current service port");
DEFINE_string(service_name, "laser_client_file", "Current geo client service name");
DEFINE_string(target_service_name, "laser_dev", "Search laser service name");
DEFINE_string(database_name, "test", "Test laser database name");
DEFINE_string(table_name, "test_raw_string", "Test laser table names");
DEFINE_int32(delimiter, 1, "Test laser delimiter");
DEFINE_bool(print, true, "Test laser delimiter");
DEFINE_string(file_name, "file", "current service host address");

DEFINE_int32(rpc_request_timeout, 10, "each request recv timeout");
DEFINE_int32(diff_range, 256, "Address diff range");
DEFINE_string(load_balance_method, "random",
              "request load balance method `random/roundrobin/localfirst/configurable_weight`");
DEFINE_string(client_request_read_mode, "mixed_read", "request read mode `leader_read/mixed_read");
DEFINE_int32(max_conn_per_server, 0, "Max connection pre server.");

class LaserCall {
 public:
  LaserCall(const std::string& target_service_name, const std::string& database_name, const std::string& table_name)
      : target_service_name_(target_service_name), database_name_(database_name), table_name_(table_name) {}
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
    local_first.setLocalIp(FLAGS_host);
    local_first.setDiffRange(FLAGS_diff_range);
    option.setLocalFirstConfig(local_first);
    return option;
  }

  void run(bool is_print, const std::string& file_name) {
    std::ifstream fin(file_name);
    if (!fin) {
      LOG(ERROR) << "Open local file fail, path:" << file_name;
      return;
    }

    std::string line_data;
    uint64_t success_count = 0;
    uint64_t fail_count = 0;
    while (getline(fin, line_data)) {
      std::vector<std::string> context_vals;
      char delimiter = static_cast<char>(FLAGS_delimiter);
      folly::split(delimiter, line_data, context_vals);
      if (context_vals.size() != 2) {
        FB_LOG_EVERY_MS(ERROR, 2000) << "Insert data format is invalid, split part is " << context_vals.size();
        continue;
      }
      laser::LaserKey key;
      getLaserKey(&key, context_vals[0]);
      if (get(key, context_vals[1], is_print)) {
        success_count++;
      } else {
        fail_count++;
      }
      FB_LOG_EVERY_MS(ERROR, 5000) << "Success number:" << success_count << " Fail count:" << fail_count;
    }
  }

 private:
  std::string target_service_name_;
  std::string database_name_;
  std::string table_name_;
  std::shared_ptr<laser::LaserClient> client_;
  laser::ClientOption client_options_;

  bool get(const laser::LaserKey& key, const std::string& value, bool is_print) {
    std::string data;
    auto ret = client_->getSync(client_options_, &data, key);
    if (!is_print) {
      return true;
    }

    if (ret != laser::Status::OK) {
      LOG(INFO) << "Call get api fail," << ret;
      return false;
    } else {
      if (data != value) {
        LOG(INFO) << "pk:" << key.get_primary_keys()[0] << " vlaue:" << data << " rvalue:" << value;
        return false;
      }
    }
    return true;
  }

  void getLaserKey(laser::LaserKey* laser_key, const std::string& key) {
    std::vector<std::string> pks({key});
    laser_key->set_database_name(database_name_);
    laser_key->set_table_name(table_name_);
    laser_key->set_primary_keys(pks);
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

  LaserCall laser_call(FLAGS_target_service_name, FLAGS_database_name, FLAGS_table_name);
  laser_call.init();

  laser_call.run(FLAGS_print, FLAGS_file_name);
  return 0;
}
