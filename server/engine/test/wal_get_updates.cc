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
 */

#include "folly/init/Init.h"
#include "gflags/gflags.h"
#include "folly/Random.h"
#include "laser/server/engine/rocksdb.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "common/metrics/metrics.h"
#include "common/metrics/transform.h"
#include "service_router/http.h"

DEFINE_int32(http_port, 11000, "Port to listen on with HTTP protocol");
DEFINE_string(host, "localhost", "IP/Hostname to bind to");
DEFINE_string(service_name, "http_server", "service name");

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  auto env = rocksdb::Env::Default();
  env->SetBackgroundThreads(2, rocksdb::Env::LOW);
  env->SetBackgroundThreads(1, rocksdb::Env::HIGH);

  auto path = folly::to<std::string>("/tmp/rocksdb_unit_test/", folly::Random::secureRand32());
  rocksdb::Options options;
  options.create_if_missing = true;
  options.env = env;
  rocksdb::DB* db = nullptr;
  rocksdb::DB::Open(options, path, &db);

  rocksdb::WriteOptions write_options;
  for (int i = 0; i < 1000000; i++) {
    std::string key = folly::to<std::string>("path", i);
    rocksdb::Slice slice_key(key.data(), key.length());
    rocksdb::WriteBatch batch;
    batch.Put(slice_key, slice_key);
    db->Write(write_options, &batch);
  }
  LOG(INFO) << "write data has complete.";

  auto timers = metrics::Metrics::getInstance()->buildTimers("test", "get_updates", 1, 0, 1000);

  std::thread http_server_thread([]() {
    folly::setThreadName("httpServerStart");
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_http_port);
  });

  uint32_t max_updates = 100;
  std::unique_ptr<rocksdb::TransactionLogIterator> iter;
  rocksdb::Status status = db->GetUpdatesSince(0, &iter);
  for (int i = 0; i < 1000000; i++) {
    metrics::Timer metric_time(timers.get());
    for (uint32_t batch_numbers = 0; batch_numbers < max_updates && iter && iter->Valid(); iter->Next()) {
      auto result = iter->GetBatch();
      result.writeBatchPtr->Data();
      batch_numbers += result.writeBatchPtr->Count();
    }
  }

  http_server_thread.join();

  return 0;
}
