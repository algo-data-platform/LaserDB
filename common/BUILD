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
  name ="version_generator",
  srcs = [
    "version.h.m4",
  ],
  outs=[
    'version.h',
  ],
  cmd="$(location //thirdparty/tools:m4_bin) --define=HEAD_COMMIT=`git rev-parse --short HEAD` $< > $@",
  tools = [
    "//thirdparty/tools:m4_bin"
  ]
)

cc_library(
  name = "util",
  srcs = [
    "util.cc",
    "console/table.cc",
    "metrics/metrics.cc",
    "metrics/transform.cc",
    "metrics/system_metrics.cc",
  ],
  hdrs = [
    "util.h",
    "console/table.h",
    "console/termcolor.h",
    ":version_generator",
    "metrics/metrics.h",
    "metrics/system_metrics.h",
    "metrics/transform.h",
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  deps=[
    "@folly//:folly",
    "@double-conversion//:double-conversion",
    "//thirdparty/boost:boost",
    "@cityhash//:cityhash",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "http_client",
  srcs = [
    "http/http_connection.cc",
    "http/http_client_manager.cc",
  ],
  hdrs = [
    "http/http_connection.h",
    "http/http_client_manager.h",
    "http/http_response.h",
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  deps=[
    "@proxygen//:proxygen",
    "@double-conversion//:double-conversion",
    "@folly//:folly",
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "consul_client",
  srcs = [
    "consul/entity.cc",
    "consul/kv_client.cc",
    "consul/agent_client.cc",
  ],
  hdrs = [
    "consul/entity.h",
    "consul/kv_client.h",
    "consul/agent_client.h",
    "consul/params.h",
    "consul/raw_client.h",
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  deps = [
    ":http_client",
    ":util"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "service_router",
  srcs=[
    "service_router/registry.cc",
    "service_router/service_router_entity.cc",
    "service_router/router.cc",
    "service_router/service_info_puller.cc",
    "service_router/thrift_client.cc",
    "service_router/thrift_server.cc",
    "service_router/load_balance.cc",
    "service_router/connection_pool.cc",
  ],
  hdrs = glob([
    "service_router/*.h" 
  ]),
  copts = [
    "-Iexternal/double-conversion/",
  ],
  deps = [
    ":consul_client",
    ":util",
    "@cityhash//:cityhash",
    "@fbthrift//:thriftcpp2",
    "@proxygen//:proxygenhttpserver"
  ],
  visibility = ["//visibility:public"],
)

cc_library(
  name = "service_router_http_server",
  srcs=[
    "service_router/http.cc"
  ],
  hdrs = [
    "service_router/http.h" 
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  deps=[
    ":service_router",
    ":http_server",
    ":util"
  ],
  visibility = ["//visibility:public"],
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
  deps=[
    "@proxygen//:proxygenhttpserver",
    "@double-conversion//:double-conversion",
    "@folly//:folly",
    ":service_router",
    ":util"
  ],
  visibility = ["//visibility:public"],
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
  deps=[
    "@folly//:folly",
    "@double-conversion//:double-conversion",
    ":http_client",
    ":util"
  ],
  visibility = ["//visibility:public"],
)

gen_fbthrift(
  name = "laser",
  src = "laser/if/laser.thrift",
  output_path = "laser/if",
  service_list = [
    "LaserService",
  ],
  options = "templates,no_metadata,include_prefix=laser/if",
)

gen_fbthrift(
  name = "laser_replication",
  src = "laser/if/laser_replication.thrift",
  output_path = "laser/if",
  service_list = [
    "Replicator",
  ],
  options = "templates,no_metadata,include_prefix=laser/if",
  thrift_include_files = [
    "laser/if/laser.thrift"
  ],
)

cc_library(
  name = "liblaser",
  srcs = [
    "laser/config_manager.cc",
    "laser/partition.cc",
    "laser/loader_source_data.cc",
    "laser/laser_entity.cc",
    "laser/format.cc",
    "laser/status.cc",
    "laser/rocksdb_config_factory.cc",
    ":laser",
    ":laser_replication",
  ],
  hdrs = [
    "laser/versioned_options.h",
    "laser/config_manager.h",
    "laser/partition.h",
    "laser/loader_source_data.h",
    "laser/laser_entity.h",
    "laser/format.h",
    "laser/status.h",
    "laser/rocksdb_config_factory.h",
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  includes = [
    "."
  ],
  deps=[
    "@folly//:folly",
    "@double-conversion//:double-conversion",
    "@rocksdb//:rocksdb",
    ":service_router",
    ":util",
  ],
  visibility = ["//visibility:public"],
)
