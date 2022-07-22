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

#pragma once

#include <iostream>
#include <mutex> // NOLINT
#include <condition_variable> // NOLINT

#include "folly/io/IOBuf.h"
#include "folly/lang/Bits.h"
#include "city.h"

#include "common/laser/laser_entity.h"
#include "common/laser/if/gen-cpp2/laser_types.h"

namespace laser {

class LockBucket {
 public:
  explicit LockBucket(int max_locks) : max_locks_(max_locks) {}
  Status tryLock(const std::string& key);
  void unLock(const std::string& key);

 private:
  int max_locks_{1024};
  std::mutex bucket_mutex_;
  std::condition_variable bucket_cv_;
  std::unordered_set<std::string> keys_;
  Status acquireLocked(const std::string& key);
};

class LockManager {
 public:
  static std::shared_ptr<LockManager> getInstance();
  LockManager();
  ~LockManager() = default;
  Status tryLock(const std::string& key);
  void unLock(const std::string& key);

 private:
  std::shared_ptr<LockBucket> getBucket(const std::string& key);
  std::vector<std::shared_ptr<LockBucket>> buckets_;
};

class ScopedKeyLock {
 public:
  explicit ScopedKeyLock(const std::string& key) : key_(key) { LockManager::getInstance()->tryLock(key_); }
  ~ScopedKeyLock() { LockManager::getInstance()->unLock(key_); }

 private:
  std::string key_;
};

}  // namespace laser
