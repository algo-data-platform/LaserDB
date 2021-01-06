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

#include <iostream>

#include "folly/init/Init.h"
#include "laser/server/engine/wdt_replicator.h"

DEFINE_string(src_directory, "/tmp/laser/leader", "send directory");
DEFINE_string(dest_directory, "/tmp/laser/follower", "rec directory");
DEFINE_string(host, "127.0.0.1", "host name");
DEFINE_string(connect_url, "", "host name");
DEFINE_bool(is_receiver, false, "Receiver");
DEFINE_int32(timeout, 10000, "timeout");

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);

  auto manager = std::make_shared<laser::WdtReplicatorManager>("test", 10);
  auto wdt = std::make_shared<laser::WdtReplicator>(manager, "test", "bar", FLAGS_timeout);
  // 另外一个 receiver
  auto wdt1 = std::make_shared<laser::WdtReplicator>(manager, "test1", "bar1", FLAGS_timeout);
  if (FLAGS_is_receiver) {
    std::string connect_url;
    wdt->receiver(&connect_url, FLAGS_host, FLAGS_dest_directory, [](auto& name_space, auto& ident, auto error) {
      LOG(ERROR) << "Transfer receiver complete,  " << facebook::wdt::errorCodeToStr(error);
    });
    std::cout << connect_url << std::endl;
    wdt1->receiver(&connect_url, FLAGS_host, FLAGS_dest_directory, [](auto& name_space, auto& ident, auto error) {
      LOG(ERROR) << "Transfer receiver complete,  " << facebook::wdt::errorCodeToStr(error);
    });
    std::cout << connect_url << std::endl;
  } else {
    wdt->sender(FLAGS_connect_url, FLAGS_src_directory, [](auto& name_space, auto& ident, auto error) {
      LOG(ERROR) << "Transfer send complete,  " << facebook::wdt::errorCodeToStr(error);
    });
  }
  manager->stop();
  return 0;
}
