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

load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

configure_make(
    name = "libevent",
    # libevent script uses it's own libtool for linking;
    # so do not specify linker tool for it
    # (otherwise, if the libtool from bazel's toolchain is supplied,
    # the build script has problems with passing output file to libtool)
    # see #315
    configure_env_vars = {
        "AR": "",
    },
    configure_options = [
        "--enable-shared=no",
        "--disable-libevent-regress",
        "--disable-openssl",
    ],
    lib_source = "@libevent//:all",
    out_lib_dir = "lib",
    visibility = ["//visibility:public"],
)
