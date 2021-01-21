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

#include <thread> // NOLINT

#include "gtest/gtest.h"
#include "folly/Singleton.h"
#include "folly/Random.h"
#include "laser/server/engine/scoped_key_lock.h"

TEST(ScopedKeyLock, noLock) {
  std::vector<std::unique_ptr<std::thread>> cal_threads;

  std::unordered_map<std::string, int> count;
  count["test"] = 0;

  for (int i = 0; i < 16; i++) {
    cal_threads.push_back(std::make_unique<std::thread>([&count]() {
      for (int i = 0; i < 10000; i++) {
        int val = count["test"];
        val++;
        count["test"] = val;
      }
    }));
  }

  for (auto& t : cal_threads) {
    t->join();
  }
  EXPECT_NE(160000, count["test"]);
}

TEST(ScopedKeyLock, uniqueKeyLock) {
  std::vector<std::unique_ptr<std::thread>> cal_threads;

  folly::SingletonVault::singleton()->registrationComplete();

  std::unordered_map<std::string, int> count;
  count["test"] = 0;

  for (int i = 0; i < 16; i++) {
    cal_threads.push_back(std::make_unique<std::thread>([&count]() {
      for (int i = 0; i < 10000; i++) {
        laser::ScopedKeyLock lock("test");
        int val = count["test"];
        val++;
        count["test"] = val;
      }
    }));
  }

  for (auto& t : cal_threads) {
    t->join();
  }
  EXPECT_EQ(160000, count["test"]);
}

TEST(ScopedKeyLock, multiKeyLock) {
  std::vector<std::unique_ptr<std::thread>> cal_threads;

  folly::SingletonVault::singleton()->registrationComplete();

  std::unordered_map<std::string, int> count;
  for (int i = 0; i < 200; i++) {
    count[folly::to<std::string>("test", i)] = 0;
  }

  for (int i = 0; i < 16; i++) {
    cal_threads.push_back(std::make_unique<std::thread>([&count]() {
      for (int i = 0; i < 10000; i++) {
        std::string key = folly::to<std::string>("test", folly::Random::secureRand64(20));
        laser::ScopedKeyLock lock(key);
        int val = count[key];
        val++;
        count[key] = val;
      }
    }));
  }

  for (auto& t : cal_threads) {
    t->join();
  }

  int sum = 0;
  for (auto& t : count) {
    sum += t.second;
  }
  EXPECT_EQ(160000, sum);
}
