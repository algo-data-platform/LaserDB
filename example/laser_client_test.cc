#include "client/laser_client.h"
#include "common/laser/status.h"
#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <memory>

DEFINE_string(target_service_name, "laser_test", "laser service name");
DEFINE_string(database_name, "test_database", "laser database name");
DEFINE_string(table_name, "test_table", "laser table name");
DEFINE_int32(rpc_request_timeout, 20, "request timeout");
DEFINE_string(test_key, "test_key", "test key");
DEFINE_string(test_value, "hello laserdb", "test value");

void getLaserKey(laser::LaserKey *laser_key, const std::string &key) {
  laser_key->set_database_name(FLAGS_database_name);
  laser_key->set_table_name(FLAGS_table_name);
  laser_key->set_primary_keys({key});
}

void getLaserKV(laser::LaserKV *laser_kv, const std::string &key,
                const std::string &val) {
  laser::LaserKey laser_key;
  getLaserKey(&laser_key, key);
  laser::LaserValue laser_value;
  laser_value.set_string_value(val);
  laser_kv->set_key(laser_key);
  laser_kv->set_value(laser_value);
}

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = true;
  folly::Init init(&argc, &argv);
  auto client = std::make_shared<laser::LaserClient>(FLAGS_target_service_name);
  client->init();
  laser::ClientOption option;
  option.setReceiveTimeoutMs(FLAGS_rpc_request_timeout);
  option.setReadMode(laser::ClientRequestReadMode::MIXED_READ);

  {
    laser::LaserKV kv;
    getLaserKV(&kv, FLAGS_test_key, FLAGS_test_value);
    auto ret = client->setSync(option, kv);
    LOG(INFO) << "test set ret: " << ret;
  }

  {
    laser::LaserKey laser_key;
    getLaserKey(&laser_key, FLAGS_test_key);
    std::string value;
    auto ret = client->getSync(option, &value, laser_key);
    LOG(INFO) << "test get ret: " << ret << ",value: " << value;
  }

  return 0;
}
