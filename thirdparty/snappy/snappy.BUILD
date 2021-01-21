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

filegroup(
  name = "snappy_header",
  srcs = glob([
    "*.h",
  ]),
)

cc_library(
  name = "snappy",
  srcs = [
    ":snappy_header",
    "snappy-c.cc",
    "snappy-sinksource.cc",
    "snappy-stubs-internal.cc",
    "snappy.cc",
  ],
  hdrs = ["snappy.h"],
  visibility = ["//visibility:public"],
)

