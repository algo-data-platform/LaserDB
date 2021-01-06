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
#

cc_library(
  name="wdt",
  srcs=[
    "util/WdtSocket.cpp",
    "util/ClientSocket.cpp",
    "util/EncryptionUtils.cpp",
    "util/DirectorySourceQueue.cpp",
    "ErrorCodes.cpp",
    "util/FileByteSource.cpp",
    "util/FileCreator.cpp",
    "Protocol.cpp",
    "WdtThread.cpp",
    "util/ThreadsController.cpp",
    "ReceiverThread.cpp",
    "Receiver.cpp",
    "WdtTransferRequest.cpp",
    "Reporting.cpp",
    "util/ThreadTransferHistory.cpp",
    "SenderThread.cpp",
    "Sender.cpp",
    "util/ServerSocket.cpp",
    "Throttler.cpp",
    "WdtOptions.cpp",
    "util/FileWriter.cpp",
    "util/TransferLogManager.cpp",
    "util/SerializationUtil.cpp",
    "util/Stats.cpp",
    "WdtBase.cpp",
    "WdtResourceController.cpp",
    "util/CommonImpl.cpp",
    "util/WdtFlags.cpp",
    "util/WdtFlags.cpp.inc",
    "Wdt.cpp"
  ],
  hdrs = glob([
    "*.h",
    "**/*.h"  
  ]),
  includes = [
    "."
  ],
	deps=[
    "@folly//:folly",
    "@com_github_gflags_gflags//:gflags",
    "@com_github_google_gtest//:gtest",
		'@//thirdparty/openssl:openssl',
    "@com_github_google_glog//:glog",
    "@double-conversion//:double-conversion",
	],
  copts = [
    "-Iexternal/double-conversion/",
  ],
  visibility = ["//visibility:public"],
)

cc_binary(
	name="wdtbin",
	srcs = [
    'wdtCmdLine.cpp'
	],
	deps = [
		':wdt',
	],
  linkopts = [
    "-ldl",
  ],
  copts = [
    "-Iexternal/double-conversion/",
  ]
)
