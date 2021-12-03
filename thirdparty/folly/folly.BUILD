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

filegroup(
    name = "cc_files",
    srcs = glob(
        [
            "folly/**/*.cpp",
            "folly/*.cpp",
        ],
        exclude = [
            "folly/build/**",
            "folly/experimental/exception_tracer/**",
            "folly/futures/exercises/**",
            "folly/logging/example/**",
            "folly/**/test/**",
            "folly/test/**",
            "folly/tools/**",
            "folly/Benchmark.cpp",
            "folly/experimental/JSONSchemaTester.cpp",
            "folly/experimental/io/HugePageUtil.cpp",
            "folly/python/**",
            "folly/cybld/folly/executor.cpp",
            "folly/experimental/io/IoUring.cpp",
            "folly/experimental/io/IoUringBackend.cpp",
            "folly/experimental/crypto/Blake2xb.cpp",
            "folly/experimental/crypto/detail/MathOperation_AVX2.cpp",
            "folly/experimental/crypto/detail/MathOperation_Simple.cpp",
            "folly/experimental/crypto/detail/MathOperation_SSE2.cpp",
            "folly/experimental/crypto/LtHash.cpp",
        ],
    ),
)

filegroup(
    name = "header_files",
    srcs = glob(
        [
            "folly/*.h",
            "folly/**/*.h",
        ],
        exclude = [
            "folly/experimental/io/IoUring.h",
            "folly/experimental/io/IoUringBackend.h",
        ],
    ),
)

cc_library(
    name = "folly",
    srcs = [
        ":header_files",
        ":cc_files",
        "folly/folly-config.h",
    ],
    hdrs = [":header_files"],
    includes = ["."],
    deps = [
        "@//thirdparty/boost:boost",
        "@//thirdparty/libevent:libevent",
        "@//thirdparty/zstd:zstd",
        "@//thirdparty/lz4:lz4",
        "@//thirdparty/zlib:libz",
        "@//thirdparty/bzip2:bzip2",
        "@fmt//:fmt",
        "@snappy//:snappy",
        "@libaio//:libaio",
        "@double-conversion//:double-conversion",
        "@com_github_google_glog//:glog",
        "@com_github_gflags_gflags//:gflags",
        "@//thirdparty/openssl:openssl",
    ],
    copts = [
        "-Iexternal/double-conversion/",
        "-Iexternal/snappy/",
    ],
    linkopts = [
        "-ldl",
        "-pthread",
    ],
    visibility = ["//visibility:public"],
)
