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
    name = "header_files",
    srcs = glob(
        [
            "wangle/acceptor/*.h",
            "wangle/bootstrap/*.h",
            "wangle/channel/*.h",
            "wangle/client/*.h",
            "wangle/client/**/*.h",
            "wangle/codec/*.h",
            "wangle/service/*.h",
            "wangle/ssl/*.h",
            "wangle/util/*.h",
        ],
    ),
)

cc_library(
    name = "wangle",
    srcs = [
        "wangle/acceptor/Acceptor.cpp",
        "wangle/acceptor/AcceptorHandshakeManager.cpp",
        "wangle/acceptor/ConnectionManager.cpp",
        "wangle/acceptor/EvbHandshakeHelper.cpp",
        "wangle/acceptor/FizzAcceptorHandshakeHelper.cpp",
        "wangle/acceptor/FizzConfigUtil.cpp",
        "wangle/acceptor/LoadShedConfiguration.cpp",
        "wangle/acceptor/ManagedConnection.cpp",
        "wangle/acceptor/SSLAcceptorHandshakeHelper.cpp",
        "wangle/acceptor/SecureTransportType.cpp",
        "wangle/acceptor/SocketOptions.cpp",
        "wangle/acceptor/TLSPlaintextPeekingCallback.cpp",
        "wangle/acceptor/TransportInfo.cpp",
        "wangle/bootstrap/ServerBootstrap.cpp",
        "wangle/channel/FileRegion.cpp",
        "wangle/channel/Pipeline.cpp",
        "wangle/client/persistence/FilePersistenceLayer.cpp",
        "wangle/client/persistence/PersistentCacheCommon.cpp",
        "wangle/client/ssl/SSLSessionCacheData.cpp",
        "wangle/client/ssl/SSLSessionCacheUtils.cpp",
        "wangle/client/ssl/SSLSessionCallbacks.cpp",
        "wangle/codec/LengthFieldBasedFrameDecoder.cpp",
        "wangle/codec/LengthFieldPrepender.cpp",
        "wangle/codec/LineBasedFrameDecoder.cpp",
        "wangle/ssl/PasswordInFileFactory.cpp",
        "wangle/ssl/SSLContextManager.cpp",
        "wangle/ssl/SSLSessionCacheManager.cpp",
        "wangle/ssl/SSLUtil.cpp",
        "wangle/ssl/ServerSSLContext.cpp",
        "wangle/ssl/TLSCredProcessor.cpp",
        "wangle/ssl/TLSTicketKeyManager.cpp",
        "wangle/util/FilePoller.cpp",
    ],
    hdrs = [
        ":header_files",
    ],
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@//thirdparty/libevent:libevent",
        "@//thirdparty/libsodium:libsodium",
        "@//thirdparty/openssl:openssl",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@double-conversion//:double-conversion",
        "@fizz",
        "@folly",
    ],
)
