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

#include "folly/portability/GFlags.h"
#include "folly/Singleton.h"

#include "scoped_key_lock.h"

namespace laser {

DEFINE_int32(lock_bucket_numbers, 1024, "Lock bucket numbers");
DEFINE_int32(lock_bucket_max_lock_number, 10000, "Each lock bucket numbers allow locks number");

folly::Singleton<LockManager> global_key_lock_manager;

std::shared_ptr<LockManager> LockManager::getInstance() { return global_key_lock_manager.try_get(); }

LockManager::LockManager() {
  for (int i = 0; i < FLAGS_lock_bucket_numbers; i++) {
    buckets_.push_back(std::make_shared<LockBucket>(FLAGS_lock_bucket_max_lock_number));
  }
}

Status LockManager::tryLock(const std::string& key) { return getBucket(key)->tryLock(key); }

void LockManager::unLock(const std::string& key) { return getBucket(key)->unLock(key); }

std::shared_ptr<LockBucket> LockManager::getBucket(const std::string& key) {
  DCHECK(buckets_.size() == FLAGS_lock_bucket_numbers);
  return buckets_.at(CityHash64(key.c_str(), key.size()) % FLAGS_lock_bucket_numbers);
}

Status LockBucket::tryLock(const std::string& key) {
  bucket_mutex_.lock();

  VLOG(10) << "start lock";
  Status result = acquireLocked(key);
  if (result != Status::OK) {
    do {
      std::unique_lock<std::mutex> lock(bucket_mutex_, std::adopt_lock);
      bucket_cv_.wait(lock);

      // 确保 unique_lock 和 bucket mutex 去掉关联关系
      lock.release();

      result = acquireLocked(key);
    } while (result != Status::OK);
  }
  VLOG(10) << "lock success";

  bucket_mutex_.unlock();
  return result;
}

void LockBucket::unLock(const std::string& key) {
  bucket_mutex_.lock();
  auto iter = keys_.find(key);
  if (iter != keys_.end()) {
    keys_.erase(iter);
  }

  bucket_mutex_.unlock();
  bucket_cv_.notify_all();
}

Status LockBucket::acquireLocked(const std::string& key) {
  if (keys_.find(key) != keys_.end()) {
    return Status::RS_BUSY;
  }

  keys_.insert(key);
  return Status::OK;
}

}  // namespace laser
