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

filegroup(
    name = "double_conversion_header",
    srcs = glob([
        "double-conversion/*.h",
    ]),
)

filegroup(
    name = "double_conversion_cc",
    srcs = glob([
        "double-conversion/*.cc",
    ]),
)

cc_library(
    name = "double-conversion",
    srcs = [
        ":double_conversion_cc",
    ],
    hdrs = [":double_conversion_header"],
    visibility = ["//visibility:public"],
)
