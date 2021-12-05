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
#

cc_library(
    name = "wdt",
    srcs = [
        "ErrorCodes.cpp",
        "Protocol.cpp",
        "Receiver.cpp",
        "ReceiverThread.cpp",
        "Reporting.cpp",
        "Sender.cpp",
        "SenderThread.cpp",
        "Throttler.cpp",
        "Wdt.cpp",
        "WdtBase.cpp",
        "WdtOptions.cpp",
        "WdtResourceController.cpp",
        "WdtThread.cpp",
        "WdtTransferRequest.cpp",
        "util/ClientSocket.cpp",
        "util/CommonImpl.cpp",
        "util/DirectorySourceQueue.cpp",
        "util/EncryptionUtils.cpp",
        "util/FileByteSource.cpp",
        "util/FileCreator.cpp",
        "util/FileWriter.cpp",
        "util/SerializationUtil.cpp",
        "util/ServerSocket.cpp",
        "util/Stats.cpp",
        "util/ThreadTransferHistory.cpp",
        "util/ThreadsController.cpp",
        "util/TransferLogManager.cpp",
        "util/WdtFlags.cpp",
        "util/WdtFlags.cpp.inc",
        "util/WdtSocket.cpp",
    ],
    hdrs = glob([
        "*.h",
        "**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@//thirdparty/openssl:openssl",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@com_github_google_gtest//:gtest",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_binary(
    name = "wdtbin",
    srcs = [
        "wdtCmdLine.cpp",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":wdt",
    ],
)
