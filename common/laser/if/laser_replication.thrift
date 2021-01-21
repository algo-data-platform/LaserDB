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
 */

include "laser.thrift"

namespace cpp2 laser

enum ReplicateType {
  FORWARD = 0,
  REVERSE = 1,
}

struct ReplicateRequest {
  # 当前本地最大的 seq 号，将要同步更新 seq_no + 1 以后的数据
  1: required i64 seq_no,

  # db name 一般是通过 partition id , table name , database name 进行hash 得出
  2: required i64 db_hash,

  # 如果请求的数据存在 server 端立即返回, 否则会等待一段时间后返回
  # 如果为 0 表示不等待直接返回
  3: required i32 max_wait_ms,

  # Server 端可能有多个更新， max_size 表示 client 单次最多批量更新的数量
  # 如果为 0 表示不限制
  4: required i64 max_size,

  5: required string version,

  # 仅用来调试，该地址会随着节点重启变化
  6: required string client_address,

  # 证明节点的唯一信息
  7: required i64 node_hash,

  # 在反向同步过程中需要同步的最大的 seq, 仅在反向同步时有用
  8: required i64 max_seq_no,

  9: required ReplicateType type, 
  10:required i64 timestamp,
}

typedef binary (cpp.type = "folly::IOBuf") IOBuf
struct Update {
  1: required IOBuf raw_data,

  # 在 leader 中更新的时间
  # 0 代表不存在更新
  2: required i64 timestamp,
}

struct ReplicateResponse {
  1: required list<Update> updates,
  2: required string version,
  3: required i64 max_seq_no,
  # 下次 pull 应该起始的 seq no
  4: required i64 next_seq_no,
  5: optional i64 timestamp,
}

struct ReplicateWdtRequest {
  1: required i64 db_hash,
  2: required string version,
  3: required string wdt_url,
  4: required i64 node_hash,
}

struct ReplicateWdtResponse {
  1: required bool send_success 
}

service Replicator {
  ReplicateResponse replicate(1:ReplicateRequest request)
      throws (1:laser.LaserException e)
  ReplicateWdtResponse replicateWdt(1:ReplicateWdtRequest request)
      throws (1:laser.LaserException e)
}
