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
#include "hdfsmonitor.h"

DEFINE_string(file_dir, "/dw_ext/ad/ads_core/laser/dev/", "the dir under monitor");
DEFINE_string(dest_path, "/tmp/laser/source_data", "the local path which used to save download files");
DEFINE_int32(time_interval, 1000, "the millseconds heartbeat");

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  FLAGS_logtostderr = true;

  auto manager = std::make_shared<hdfs::HdfsMonitorManager>(10);
  {
    auto monitor = std::make_shared<hdfs::HdfsMonitor>(FLAGS_file_dir, FLAGS_dest_path, FLAGS_time_interval,
                                                       [](const std::vector<std::string>& files) {
                                                         for (auto& file : files) {
                                                           LOG(INFO) << file;
                                                         }
                                                       },
                                                       manager);

    for (int i = 0; i < 100; i++) {
      monitor->addFilter({"0/", "1/"});
    }
    monitor->start();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    for (int i = 0; i < 100; i++) {
      monitor->addFilter({"2/", "3/"});
    }

    monitor->delFilter({"2/"});
    monitor->destroy();
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
