#
# Copyright (c) 2020-present, Weibo, Inc.  All rights reserved.
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

cc_binary(
	name = "laser_proxy",
	srcs=[
    "laser_proxy.cc",
    "laser_proxy_service.h",
    "laser_proxy_service.cc",
    "proxy_config.h",
    "proxy_config.cc",
    "redis.h",
    "redis.cc",
    "redis_command_process.h",
    "redis_command_process.cc",
	],
	deps=[
    "//client:laser_client",
    "//common:service_router_http_server"
	],
  linkopts = [
    '-ldl'
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  visibility = ["//visibility:public"],
)
