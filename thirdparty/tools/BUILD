#
# Copyright 2018 Google LLC
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

"""
Rules for adding './configure && make' style dependencies.
"""

load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")
load(":ar_wrapper.bzl", "ar_wrapper")

licenses(["notice"])  # Apache v2.0

exports_files([
    "bison.bzl",
    "flex.bzl",
    "fbthrift.bzl",
])

ar_wrapper(
    name = "ar_wrapper",
    visibility = ["//visibility:public"],
)

configure_make(
    name = "m4",
    binaries = [
        "m4",
    ],
    configure_env_vars = {
        "AR": "ar_wrapper",
    },
    configure_options = [
        "--disable-nls",
    ],
    lib_source = "@m4//:all",
    tools_deps = [":ar_wrapper"],
)

filegroup(
    name = "m4_bin",
    srcs = [":m4"],
    output_group = "m4",
    visibility = ["//visibility:public"],
)

configure_make(
    name = "bison",
    binaries = [
        "bison",
        "yacc",
    ],
    lib_source = "@bison//:all",
    static_libraries = ["liby.a"],
)

filegroup(
    name = "bison_bin",
    srcs = [":bison"],
    output_group = "bison",
    visibility = ["//visibility:public"],
)

configure_make(
    name = "flex",
    binaries = [
        "flex",
    ],
    lib_source = "@flex//:all",
)

filegroup(
    name = "flex_bin",
    srcs = [":flex"],
    output_group = "flex",
    visibility = ["//visibility:public"],
)
