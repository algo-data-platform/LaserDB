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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#include "folly/String.h"

#include "database_meta_info.h"
#include "datapath_manager.h"


namespace laser {

constexpr static char DATABASE_META_INFO_PATH[] = "meta_info";
constexpr static char DATABASE_META_VERSION_SPLIT[] = "_";
constexpr static char DATABASE_META_DELTA_VERSION_SPLIT[] = ",";
constexpr static char DATABASE_META_DELTA_VERSION_KEY[] = "delta";

bool DatabaseMetaInfo::init(const rocksdb::Options& options) {
  std::string data_dir =
      folly::to<std::string>(DataPathManager::getDataBase(), DATA_PATH_MANAGER_DIR_SPLIT, DATABASE_META_INFO_PATH);
  auto replication_db = std::make_shared<laser::ReplicationDB>(data_dir, options);
  meta_db_ = std::make_shared<laser::RocksDbEngine>(replication_db);
  if (!meta_db_->open()) {
    LOG(INFO) << "Open database meta db fail";
    return false;
  }
  return true;
}

void DatabaseMetaInfo::updateVersion(std::shared_ptr<Partition> partition, const std::string& version) {
  int64_t partition_key = partition->getPartitionHash();
  std::vector<std::string> primary_keys({folly::to<std::string>(partition_key)});
  std::vector<std::string> column_names;
  LaserKeyFormat key(primary_keys, column_names);
  Status status = meta_db_->set(key, version);

  if (status != Status::OK) {
    LOG(INFO) << "Update partition:" << *partition << " version:" << version
              << " fail, status:" << static_cast<uint32_t>(status);
  }
}

const std::string DatabaseMetaInfo::getVersion(std::shared_ptr<Partition> partition) {
  LaserValueRawString value;
  int64_t partition_key = partition->getPartitionHash();
  std::vector<std::string> primary_keys({folly::to<std::string>(partition_key)});
  std::vector<std::string> column_names;
  LaserKeyFormat key(primary_keys, column_names);
  Status status = meta_db_->get(&value, key);

  if (status == Status::OK) {
    return value.getValue();
  }
  LOG(INFO) << "Get partition:" << *partition << " version fail, status:" << static_cast<uint32_t>(status);

  std::string version = generateVersion(partition);

  LOG(INFO) << "Create new version:" << version << " for partition: " << *partition;
  updateVersion(partition, version);
  return version;
}

void DatabaseMetaInfo::updateDeltaVersions(std::shared_ptr<Partition> partition,
                                           const std::vector<std::string>& versions) {
  int64_t partition_key = partition->getPartitionHash();
  std::vector<std::string> primary_keys({folly::to<std::string>(partition_key)});
  std::vector<std::string> column_names({DATABASE_META_DELTA_VERSION_KEY});
  LaserKeyFormat key(primary_keys, column_names);

  std::string delta_version;
  folly::join(DATABASE_META_DELTA_VERSION_SPLIT, versions, delta_version);
  Status status = meta_db_->set(key, delta_version);

  if (status != Status::OK) {
    LOG(INFO) << "Update delta version fail, partition:" << *partition << " delta_version:" << delta_version
              << ", status:" << static_cast<uint32_t>(status);
  }
}

const std::vector<std::string> DatabaseMetaInfo::getDeltaVersions(std::shared_ptr<Partition> partition) {
  LaserValueRawString value;
  int64_t partition_key = partition->getPartitionHash();
  std::vector<std::string> primary_keys({folly::to<std::string>(partition_key)});
  std::vector<std::string> column_names({DATABASE_META_DELTA_VERSION_KEY});
  LaserKeyFormat key(primary_keys, column_names);
  Status status = meta_db_->get(&value, key);

  std::vector<std::string> delta_versions;
  if (status != Status::OK) {
    LOG(INFO) << "Get delta version fail, partition:" << *partition << ", status:" << static_cast<uint32_t>(status);
    return delta_versions;
  }

  folly::split(DATABASE_META_DELTA_VERSION_SPLIT, value.getValue(), delta_versions);
  return delta_versions;
}

const std::string DatabaseMetaInfo::generateVersion(std::shared_ptr<Partition> partition) {
  // 当当前节点不存在该分区数据时，如果是 leader 将默认 version 是 default
  // 如果是follower version 是按照时间hash 的随机 version, 不是 default 的原因是为了通过启动时
  // 由于版本不一致会触发全量同步，由于这一约束，后续扩容必须先通过 follower 角色扩容，等对齐后再切换主从
  if (partition->getRole() == DBRole::LEADER) {
    std::string version = "default";
    return version;
  }

  time_t current_time = common::currentTimeInMs() / 1000;
  struct tm local_tm;
  localtime_r(&current_time, &local_tm);
  char buff[1024];
  strftime(buff, 1024, "%Y%m%d%H%M%S", &local_tm);
  std::string time = folly::to<std::string>(static_cast<uint64_t>(current_time));
  std::string hash_postfix =
      folly::to<std::string>(CityHash64WithSeed(time.data(), time.size(), folly::Random::secureRand32()));
  return folly::to<std::string>(std::string(buff), DATABASE_META_VERSION_SPLIT, hash_postfix);
}

bool DatabaseMetaInfo::deleteVersion(std::shared_ptr<Partition> partition) {
  LaserValueRawString value;
  int64_t partition_key = partition->getPartitionHash();
  std::vector<std::string> primary_keys({folly::to<std::string>(partition_key)});
  std::vector<std::string> column_names;
  LaserKeyFormat key(primary_keys, column_names);
  Status status = meta_db_->get(&value, key);
  if (status != Status::OK) {
    return false;
  }
  status = meta_db_->delkey(key);
  if (status == Status::OK) {
    VLOG(5) << "Delete partition: " << *partition << " successfully.";
    return true;
  }
  LOG(ERROR) << "Delete partition: " << *partition << " version fail, status: " << static_cast<uint32_t>(status);
  return false;
}

}  // namespace laser
