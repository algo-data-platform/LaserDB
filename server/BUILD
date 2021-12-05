#
# Copyright 2020 Weibo Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# @author ZhongXiu Hao <nmred.hao@gmail.com>
# @author liubang <it.liubang@gmail.com>

cc_library(
    name = "laser_server",
    srcs = [
        "database_manager.cc",
        "database_meta_info.cc",
        "datapath_manager.cc",
        "engine/expire_filter.cc",
        "engine/replication_db.cc",
        "engine/replicator_manager.cc",
        "engine/replicator_service.cc",
        "engine/rocksdb.cc",
        "engine/scoped_key_lock.cc",
        "engine/wdt_replicator.cc",
        "http_service.cc",
        "laser_service.cc",
        "partition_handler.cc",
        "table_monitor.cc",
    ],
    hdrs = [
        "database_manager.h",
        "database_meta_info.h",
        "datapath_manager.h",
        "engine/expire_filter.h",
        "engine/replication_db.h",
        "engine/replicator_manager.h",
        "engine/replicator_service.h",
        "engine/rocksdb.h",
        "engine/scoped_key_lock.h",
        "engine/wdt_replicator.h",
        "http_service.h",
        "laser_service.h",
        "partition_handler.h",
        "table_monitor.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        "@//common:hdfs",
        "@//common:liblaser",
        "@//common:service_router_http_server",
        "@wdt",
    ],
)

cc_binary(
    name = "laser",
    srcs = [
        "laser.cc",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":laser_server",
    ],
)
