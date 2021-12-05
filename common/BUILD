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

load("@//thirdparty/tools:fbthrift.bzl", "gen_fbthrift")

genrule(
    name = "version_generator",
    srcs = [
        "version.h.m4",
    ],
    outs = [
        "version.h",
    ],
    cmd = "$(location //thirdparty/tools:m4_bin) --define=HEAD_COMMIT=`git rev-parse --short HEAD` $< > $@",
    tools = [
        "//thirdparty/tools:m4_bin",
    ],
)

cc_library(
    name = "util",
    srcs = [
        "console/table.cc",
        "metrics/metrics.cc",
        "metrics/system_metrics.cc",
        "metrics/transform.cc",
        "util.cc",
    ],
    hdrs = [
        "console/table.h",
        "console/termcolor.h",
        "metrics/metrics.h",
        "metrics/system_metrics.h",
        "metrics/transform.h",
        "util.h",
        ":version_generator",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//thirdparty/boost",
        "@cityhash",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "http_client",
    srcs = [
        "http/http_client_manager.cc",
        "http/http_connection.cc",
    ],
    hdrs = [
        "http/http_client_manager.h",
        "http/http_connection.h",
        "http/http_response.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@double-conversion//:double-conversion",
        "@folly",
        "@proxygen",
    ],
)

cc_library(
    name = "consul_client",
    srcs = [
        "consul/agent_client.cc",
        "consul/entity.cc",
        "consul/kv_client.cc",
    ],
    hdrs = [
        "consul/agent_client.h",
        "consul/entity.h",
        "consul/kv_client.h",
        "consul/params.h",
        "consul/raw_client.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":http_client",
        ":util",
    ],
)

cc_library(
    name = "service_router",
    srcs = [
        "service_router/connection_pool.cc",
        "service_router/load_balance.cc",
        "service_router/registry.cc",
        "service_router/router.cc",
        "service_router/service_info_puller.cc",
        "service_router/service_router_entity.cc",
        "service_router/thrift_client.cc",
        "service_router/thrift_server.cc",
    ],
    hdrs = glob([
        "service_router/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":consul_client",
        ":util",
        "@cityhash",
        "@fbthrift//:thriftcpp2",
        "@proxygen//:proxygenhttpserver",
    ],
)

cc_library(
    name = "service_router_http_server",
    srcs = [
        "service_router/http.cc",
    ],
    hdrs = [
        "service_router/http.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":http_server",
        ":service_router",
        ":util",
    ],
)

cc_library(
    name = "http_server",
    srcs = [
        "http/http_server_manager.cc",
    ],
    hdrs = [
        "http/http_server_manager.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":service_router",
        ":util",
        "@double-conversion//:double-conversion",
        "@folly",
        "@proxygen//:proxygenhttpserver",
    ],
)

cc_library(
    name = "hdfs",
    srcs = [
        "hdfs/hdfs.cc",
        "hdfs/hdfsmonitor.cc",
        "hdfs/http_client.cc",
    ],
    hdrs = [
        "hdfs/hdfs.h",
        "hdfs/hdfsmonitor.h",
        "hdfs/http_client.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":http_client",
        ":util",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

gen_fbthrift(
    name = "laser",
    src = "laser/if/laser.thrift",
    options = "templates,no_metadata,include_prefix=laser/if",
    output_path = "laser/if",
    service_list = [
        "LaserService",
    ],
)

gen_fbthrift(
    name = "laser_replication",
    src = "laser/if/laser_replication.thrift",
    options = "templates,no_metadata,include_prefix=laser/if",
    output_path = "laser/if",
    service_list = [
        "Replicator",
    ],
    thrift_include_files = [
        "laser/if/laser.thrift",
    ],
)

cc_library(
    name = "liblaser",
    srcs = [
        "laser/config_manager.cc",
        "laser/format.cc",
        "laser/laser_entity.cc",
        "laser/loader_source_data.cc",
        "laser/partition.cc",
        "laser/rocksdb_config_factory.cc",
        "laser/status.cc",
        ":laser",
        ":laser_replication",
    ],
    hdrs = [
        "laser/config_manager.h",
        "laser/format.h",
        "laser/laser_entity.h",
        "laser/loader_source_data.h",
        "laser/partition.h",
        "laser/rocksdb_config_factory.h",
        "laser/status.h",
        "laser/versioned_options.h",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":service_router",
        ":util",
        "@double-conversion//:double-conversion",
        "@folly",
        "@rocksdb",
    ],
)
