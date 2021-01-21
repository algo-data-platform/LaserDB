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

#include "rocksdb/compaction_filter.h"

DEFINE_int32(http_port, 11000, "Port to listen on with HTTP protocol");
DEFINE_string(host, "localhost", "IP/Hostname to bind to");
DEFINE_string(service_name, "http_server", "service name");


class StringsFilter : public rocksdb::CompactionFilter {
 public:
  StringsFilter() = default;
  bool Filter(int level, const rocksdb::Slice& key,
              const rocksdb::Slice& value,
              std::string* new_value, bool* value_changed) const override {
    return true;
  }

  const char* Name() const override { return "StringsFilter"; }
};

class StringsFilterFactory : public rocksdb::CompactionFilterFactory {
 public:
  StringsFilterFactory() = default;
  std::unique_ptr<rocksdb::CompactionFilter> CreateCompactionFilter(
    const rocksdb::CompactionFilter::Context& context) override {
    return std::unique_ptr<rocksdb::CompactionFilter>(new StringsFilter());
  }
  const char* Name() const override {
    return "StringsFilterFactory";
  }
};

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  auto env = rocksdb::Env::Default();
  env->SetBackgroundThreads(2, rocksdb::Env::LOW);
  env->SetBackgroundThreads(1, rocksdb::Env::HIGH);

  auto path = folly::to<std::string>("/tmp/rocksdb_unit_test/", folly::Random::secureRand32());
  rocksdb::Options options;
  options.compaction_filter_factory = std::make_shared<StringsFilterFactory>();
  options.create_if_missing = true;
  options.env = env;
  rocksdb::DB* db = nullptr;
  rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
  if (!status.ok()) {
    LOG(INFO) << "Create db fail, reason:" << status.ToString();
    return 1;
  }

  LOG(INFO) << "start write:" << static_cast<uint64_t>(db->GetLatestSequenceNumber());
  rocksdb::WriteOptions write_options;
  for (int i = 0; i < 1000000; i++) {
    std::string key = folly::to<std::string>("path", i);
    rocksdb::Slice slice_key(key.data(), key.length());
    rocksdb::WriteBatch batch;
    batch.Put(slice_key, slice_key);
    db->Write(write_options, &batch);
  }
  LOG(INFO) << "end write:" << static_cast<uint64_t>(db->GetLatestSequenceNumber());
  rocksdb::CompactRangeOptions coptions;
  db->CompactRange(coptions, nullptr, nullptr);
  LOG(INFO) << "end compact:" << static_cast<uint64_t>(db->GetLatestSequenceNumber());

  uint64_t count = 0;
  for (int i = 0; i < 1000000; i++) {
    std::string key = folly::to<std::string>("path", i);
    rocksdb::Slice slice_key(key.data(), key.length());
    rocksdb::ReadOptions read;
    std::string value;
    rocksdb::Status status = db->Get(read, slice_key, &value);
    if (status.ok()) {
      count++;
    }
  }
  LOG(INFO) << "end compact:" << static_cast<uint64_t>(db->GetLatestSequenceNumber());
  LOG(INFO) << "key size:" << count;

  LOG(INFO) << "write data has complete.";

  std::thread http_server_thread([]() {
    folly::setThreadName("httpServerStart");
    service_router::httpServiceServer(FLAGS_service_name, FLAGS_host, FLAGS_http_port);
  });

  http_server_thread.join();

  return 0;
}
