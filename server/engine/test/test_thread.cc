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
 */

#include "folly/init/Init.h"
#include "gflags/gflags.h"
#include "folly/Random.h"
#include "laser/server/engine/rocksdb.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "rocksdb/sst_file_manager.h"

int main(int argc, char** argv) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);

  auto env = rocksdb::Env::Default();
  env->SetBackgroundThreads(2, rocksdb::Env::LOW);
  env->SetBackgroundThreads(1, rocksdb::Env::HIGH);

  // std::vector<std::shared_ptr<laser::RocksDbEngine>> dbs;
  // for (int i = 0; i < 1000; i++) {
  //  auto path = folly::to<std::string>("/tmp/rocksdb_unit_test/", folly::Random::secureRand32());
  //  rocksdb::Options options;
  //  options.create_if_missing = true;
  //  options.env = env;
  //  auto db = std::make_shared<laser::ReplicationDB>(path, options);
  //  auto rocksdb = std::make_shared<laser::RocksDbEngine>(db);
  //  rocksdb->open();
  //  dbs.push_back(rocksdb);
  //}

  rocksdb::Options options;
  options.create_if_missing = true;
  options.max_background_jobs = 1;
  options.env = env;
  options.sst_file_manager =
      std::shared_ptr<rocksdb::SstFileManager>(rocksdb::NewSstFileManager(env, options.info_log));
  std::vector<rocksdb::DB*> dbs;
  for (int i = 0; i < 500; i++) {
    auto path = folly::to<std::string>("/tmp/rocksdb_unit_test/", i);
    rocksdb::DB* db = nullptr;
    rocksdb::DB::Open(options, path, &db);
    dbs.push_back(db);
  }

  while (1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return 0;
}
