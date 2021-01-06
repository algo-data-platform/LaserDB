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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#pragma once

#include "city.h"
#include "rocksdb/options.h"

namespace laser {

class VersionedOptions {
 public:
  VersionedOptions() : options_(nullptr), version_hash_(0) {}
  VersionedOptions(const std::shared_ptr<rocksdb::Options> options, const std::string& config_name,
                   uint32_t config_version)
      : options_(options) {
    updateVersionHash(config_name, config_version);
  }
  ~VersionedOptions() {}

  VersionedOptions(const VersionedOptions& other)
      : options_(std::make_shared<rocksdb::Options>(*other.options_)), version_hash_(other.version_hash_) {}

  VersionedOptions& operator=(const VersionedOptions& other) {
    if (this != &other) {
      options_ = std::make_shared<rocksdb::Options>(*other.options_);
      version_hash_ = other.version_hash_;
    }
    return *this;
  }

  VersionedOptions(VersionedOptions&& other) : options_(nullptr), version_hash_(0) { *this = std::move(other); }

  VersionedOptions& operator=(VersionedOptions&& other) {
    if (this != &other) {
      options_ = other.options_;
      version_hash_ = other.version_hash_;
      other.options_ = nullptr;
      other.version_hash_ = 0;
    }
    return *this;
  }

  rocksdb::Options getOptions() const { return *options_; }

  void setOptions(const std::shared_ptr<rocksdb::Options>& options) { options_ = options; }

  uint64_t getVersionHash() const { return version_hash_; }

  void updateVersionHash(const std::string& config_name, uint32_t config_version) {
    version_hash_ = CityHash64WithSeed(config_name.c_str(), config_name.length(), config_version);
  }

 private:
  std::shared_ptr<rocksdb::Options> options_;
  uint64_t version_hash_;
};
}  // namespace laser
