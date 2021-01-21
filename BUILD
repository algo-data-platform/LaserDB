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

load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar")

package(default_visibility = ["//visibility:public"])

PLATFORM_ARCH_STRING = "linux-amd64"

filegroup(
    name = "tools-config-srcs",
    srcs = glob([
        "tools/config/*.py",
    ]),
    visibility = ["//visibility:private"],
)

filegroup(
    name = "tools-config-open_source_test-srcs",
    srcs = glob([
        "tools/config/**/*.py",
    ]),
    visibility = ["//visibility:private"],
)

filegroup(
    name = "tools-srcs",
    srcs = glob([
        "tools/load_config.py",
        "tools/requirements.txt",
        "tools/README",
    ]),
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laserdb_bin",
    srcs = [
        "//example:laser_key_test",
        "//proxy:laser_proxy",
        "//server:laser",
    ],
    mode = "0755",
    package_dir = "bin",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laserdb_tools",
    srcs = [
        ":tools-srcs",
    ],
    mode = "0644",
    package_dir = "tools",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laserdb_tools_config",
    srcs = [
        ":tools-config-srcs",
    ],
    mode = "0644",
    package_dir = "tools/config",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laserdb_tools_config_open_source_test",
    srcs = [
        ":tools-config-open_source_test-srcs",
    ],
    mode = "0644",
    package_dir = "tools/config/open_source_test",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laser_control_backend",
    srcs = [
        "//control:laser_control",
    ],
    package_dir = "bin/",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "_laser_control_frontend",
    srcs = [
        "//control/frontend:build",
    ],
    mode = "0664",
    package_dir = "frontend/",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "laser_control",
    srcs = [
        "//control:templates",
    ],
    extension = "tar.gz",
    mode = "0664",
    package_dir = "laser_control",
    visibility = ["//visibility:private"],
    deps = [
        ":_laser_control_backend",
        ":_laser_control_frontend",
    ],
)

pkg_tar(
    name = "laser_batch_update_manager",
    srcs = [
        "//control/batch_update_manager:laser_batch_update_manager",
    ],
    extension = "tar.gz",
    package_dir = "laser_batch_update_manager/bin",
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "laserdb-%s" % PLATFORM_ARCH_STRING,
    extension = "tar.gz",
    package_dir = "laserdb",
    deps = [
        ":_laserdb_bin",
        ":_laserdb_tools",
        ":_laserdb_tools_config",
        ":_laserdb_tools_config_open_source_test",
    ],
)

filegroup(
    name = "release-tars",
    srcs = [
        ":laserdb-%s" % PLATFORM_ARCH_STRING,
    ],
)
