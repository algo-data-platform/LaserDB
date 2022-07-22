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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 * @author Mingrui Zhang <zmr13140@gmail.com>
 */

#include "status.h"

namespace laser {

StatusMetaInfo StatusInfo::meta_info_list[] = {
    {Status::OK, "OK", "operator ok."},
    {Status::RS_NOT_FOUND, "RS_NOT_FOUND", "rocksdb engine operator not found."},
    {Status::RS_CORRUPTION, "RS_CORRUPTION", "rocksdb engine operator corruption."},
    {Status::RS_NOT_SUPPORTED, "RS_NOT_SUPPORTED", "rocksdb engine operator not supported."},
    {Status::RS_INVALID_ARGUMENT, "RS_INVALID_ARGUMENT", "rocksdb engine operator invalid argument."},
    {Status::RS_IO_ERROR, "RS_IO_ERROR", "rocksdb engine operator io error."},
    {Status::RS_MERGE_INPROGRESS, "RS_MERGE_INPROGRESS", "rocksdb engine merge inprogress."},
    {Status::RS_IN_COMPLETE, "RS_IN_COMPLETE", "rocksdb engine in complete."},
    {Status::RS_SHUTDOWN_INPROGRESS, "RS_SHUTDOWN_INPROGRESS", "rocksdb engine shutdown improgress."},
    {Status::RS_TIMEDOUT, "RS_TIMEDOUT", "rocksdb engine operator timeout."},
    {Status::RS_ABORTED, "RS_ABORTED", "rocksdb engine operator aborted."},
    {Status::RS_BUSY, "RS_BUSY", "rocksdb engine operator busy."},
    {Status::RS_EXPIRED, "RS_EXPIRED", "rocksdb engine operator expired."},
    {Status::RS_TRYAGAIN, "RS_TRYAGAIN", "rocksdb engine operator try again."},
    {Status::RS_COMPACTION_TOO_LARGE, "RS_COMPACTION_TOO_LARGE", "rocksdb engine compaction too large."},
    {Status::RS_ERROR, "RS_ERROR", "rocksdb engine operator error."},
    {Status::RS_EMPTY, "RS_EMPTY", "rocksdb engine operator empty."},
    {Status::RS_WRITE_IN_FOLLOWER, "RS_WRITE_IN_FOLLOWER", "rocksdb engine write operator in follower not allow."},
    {Status::RS_KEY_EXPIRE, "RS_KEY_EXPIRE", "key has expire"},
    {Status::RS_KEY_EXISTS, "RS_KEY_EXISTS", "key has exists"},
    {Status::RS_PART_FAILED, "RS_PART_FAILED", "mset/mget/mdel keys part failed"},
    {Status::RS_TRAFFIC_RESTRICTION, "RS_TRAFFIC_RESTRICTION", "trigger service traffic restriction"},
    {Status::RS_OPERATION_DENIED, "RS_OPERATION_DENIED", "request is denied cause has no access to this operation"},
    {Status::SERVICE_NOT_EXISTS_PARTITION, "SERVICE_NOT_EXISTS_PARTITION", "service not exists partition db."},
    {Status::SERVICE_UNION_DATA_TYPE_INVALID, "SERVICE_UNION_DATA_TYPE_INVALID",
     "service response union data type invalid."},
    {Status::CLIENT_THRIFT_CALL_ERROR, "CLIENT_THRIFT_CALL_ERROR", "client call thrift server error."},
    {Status::CLIENT_THRIFT_CALL_NO_SHARD_ID, "CLIENT_THRIFT_CALL_NO_SHARD_ID", "client get shard id fail."},
    {Status::CLIENT_UNION_DATA_TYPE_INVALID, "CLIENT_UNION_DATA_TYPE_INVALID",
     "client request union data type invalid."},
    {Status::CLIENT_THRIFT_CALL_TIMEOUT, "CLIENT_THRIFT_CALL_TIMEOUT", "client call server timeout."},
    {Status::CLIENT_THRIFT_FUTURE_TIMEOUT, "CLIENT_THRIFT_FUTURE_TIMEOUT", "client call server future timeout."},
    {Status::RP_SOURCE_NOT_FOUND, "RP_SOURCE_NOT_FOUND", "replicate source db not found."},
    {Status::RP_ROLE_ERROR, "RP_ROLE_ERROR", "replicate role error."},
    {Status::RP_SOURCE_READ_ERROR, "RP_SOURCE_READ_ERROR", "replicate source read error."},
    {Status::RP_SOURCE_DB_REMOVED, "RP_SOURCE_DB_REMOVED", "replicate source db has removed."},
    {Status::RP_SOURCE_WAL_LOG_REMOVED, "RP_SOURCE_WAL_LOG_REMOVED", "replicate source wal log has removed."},
    {Status::GENERATOR_TABLE_NOT_EXISTS, "GENERATOR_TABLE_NOT_EXISTS", "generator table not exists"},
    {Status::GENERATOR_GET_TABLE_LOCK_FAIL, "GENERATOR_GET_TABLE_LOCK_FAIL", "generator get table lock falgs fail"},
    {Status::GENERATOR_TABLE_PROCESSING, "GENERATOR_TABLE_PROCESSING", "generator processing"},
    {Status::GENERATOR_TABLE_SET_QUEUE_FAIL, "GENERATOR_TABLE_SET_QUEUE_FAIL", "generator set queue fail"},
    {Status::GENERATOR_TABLE_SET_HASH_FAIL, "GENERATOR_TABLE_SET_HASH_FAIL", "generator set hash fail"},
    {Status::GENERATOR_TABLE_SET_LOCK_FAIL, "GENERATOR_TABLE_SET_LOCK_FAIL", "generator set lock fail"},
    {Status::GENERATOR_TABLE_DEL_LOCK_FAIL, "GENERATOR_TABLE_DEL_LOCK_FAIL", "generator del lock fail"},
    {Status::GENERATOR_TABLE_DEL_QUEUE_FAIL, "GENERATOR_TABLE_DEL_QUEUE_FAIL", "generator del queue fail"},
    {Status::UNKNOWN_ERROR, "UNKNOWN_ERROR", "laser unknown error."},
};

const std::string statusToMessage(const Status& status) {
  StatusMetaInfo* info = &StatusInfo::meta_info_list[0];
  while (info->status != status && info->status != Status::UNKNOWN_ERROR) info++;
  return info->message;
}

const std::string statusToName(const Status& status) {
  StatusMetaInfo* info = &StatusInfo::meta_info_list[0];
  while (info->status != status && info->status != Status::UNKNOWN_ERROR) info++;
  return info->status_name;
}

std::ostream& operator<<(std::ostream& os, const Status& status) {
  os << " status:"
     << "[" << statusToName(status) << "] " << statusToMessage(status);
  return os;
}

void throwLaserException(const Status& status, const std::string& message) {
  LaserException ex;
  ex.status = status;
  ex.message = message + " status:[" + statusToName(status) + "] " + statusToMessage(status);
  throw ex;
}

const LaserException createLaserException(const Status& status, const std::string& message) {
  LaserException ex;
  ex.status = status;
  ex.message = message + " status:[" + statusToName(status) + "] " + statusToMessage(status);
  return ex;
}

}  // namespace laser
