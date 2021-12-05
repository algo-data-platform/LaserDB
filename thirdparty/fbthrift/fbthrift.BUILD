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

load("@//thirdparty/tools:bison.bzl", "genyacc")
load("@//thirdparty/tools:flex.bzl", "genlex")
load("@//thirdparty/tools:fbthrift.bzl", "gen_fbthrift")
load("@io_bazel_rules_go//go:def.bzl", "go_library")

cc_binary(
    name = "compiler_generate_build_templates",
    srcs = [
        "thrift/compiler/generate/build_templates.cc",
    ],
    linkopts = [
        "-static-libstdc++",
        "-static-libgcc",
    ],
    linkstatic = 1,
    deps = [
        "@//thirdparty/boost:boost",
    ],
)

genrule(
    name = "templates_cc",
    srcs = [
        "thrift/compiler/generate/templates",
    ],
    outs = [
        "thrift/compiler/generate/templates.cc",
    ],
    cmd = "$(location :compiler_generate_build_templates) $< > $@",
    exec_tools = [
        ":compiler_generate_build_templates",
    ],
)

cc_library(
    name = "compiler_generate_templates",
    srcs = [
        ":templates_cc",
    ],
    hdrs = [
        "thrift/compiler/generate/templates.h",
    ],
    includes = [
        ".",
    ],
    linkopts = [
        "-static-libstdc++",
        "-static-libgcc",
    ],
    linkstatic = 1,
)

genyacc(
    name = "parser",
    src = "thrift/compiler/parse/thrifty.yy",
    extra_options = [
        "--skeleton=lalr1.cc",
    ],
    header_out = "thrifty.hh",
    source_out = "thrifty.cc",
)

genlex(
    name = "parserl",
    src = "thrift/compiler/parse/thriftl.ll",
    out = "thriftl.cc",
)

cc_library(
    name = "compiler_ast",
    srcs = [
        "thrift/compiler/ast/base_types.cc",
        "thrift/compiler/ast/t_program.cc",
        "thrift/compiler/ast/t_scope.cc",
        "thrift/compiler/ast/t_struct.cc",
        "thrift/compiler/ast/t_type.cc",
        "thrift/compiler/ast/t_typedef.cc",
    ],
    hdrs = glob([
        "thrift/compiler/*.h",
        "thrift/compiler/**/*.h",
    ]),
    includes = [
        ".",
    ],
    deps = [
        "@//thirdparty/openssl:openssl",
    ],
)

cc_library(
    name = "compiler_base",
    srcs = [
        "thrift/compiler/common.cc",
        "thrift/compiler/parse/parsing_driver.cc",
        "thrift/compiler/platform.cc",
        "thrift/compiler/util.cc",
        ":parser",
        ":parserl",
    ],
    hdrs = glob([
        "thrift/compiler/*.h",
        "thrift/compiler/**/*.h",
    ]),
    defines = [
        'THRIFTY_HH=\\\"thrifty.hh\\\"',
    ],
    includes = [
        ".",
    ],
    deps = [
        ":compiler_ast",
        "@//thirdparty/boost:boost",
    ],
)

cc_library(
    name = "compiler_lib",
    srcs = [
        "thrift/compiler/lib/cpp2/util.cc",
        "thrift/compiler/lib/java/util.cc",
    ],
    hdrs = glob([
        "thrift/compiler/*.h",
        "thrift/compiler/**/*.h",
    ]),
    includes = [
        ".",
    ],
    deps = [
        ":compiler_ast",
        "@//thirdparty/boost:boost",
    ],
)

cc_library(
    name = "mustache_lib",
    srcs = [
        "thrift/compiler/mustache/mstch.cpp",
        "thrift/compiler/mustache/render_context.cpp",
        "thrift/compiler/mustache/state/in_section.cpp",
        "thrift/compiler/mustache/state/outside_section.cpp",
        "thrift/compiler/mustache/template_type.cpp",
        "thrift/compiler/mustache/token.cpp",
        "thrift/compiler/mustache/utils.cpp",
    ],
    hdrs = glob([
        "thrift/compiler/*.h",
        "thrift/compiler/**/*.h",
    ]),
    includes = [
        ".",
    ],
    deps = [
        "@//thirdparty/boost:boost",
    ],
)

cc_library(
    name = "compiler_generators",
    srcs = glob(
        [
            "thrift/compiler/generate/*.cc",
        ],
        exclude = [
            "thrift/compiler/generate/build_templates.cc",
        ],
    ),
    hdrs = glob([
        "thrift/compiler/*.h",
        "thrift/compiler/**/*.h",
    ]),
    includes = [
        ".",
    ],
    deps = [
        ":compiler_base",
        ":compiler_generate_templates",
        ":compiler_lib",
        ":mustache_lib",
        "@//thirdparty/boost:boost",
        "@//thirdparty/openssl:openssl",
    ],
    alwayslink = 1,
)

cc_binary(
    name = "thrift1",
    srcs = [
        "thrift/compiler/ast/visitor.cc",
        "thrift/compiler/compiler.cc",
        "thrift/compiler/main.cc",
        "thrift/compiler/mutator/mutator.cc",
        "thrift/compiler/validator/diagnostic.cc",
        "thrift/compiler/validator/validator.cc",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":compiler_ast",
        ":compiler_base",
        ":compiler_generators",
        "@//thirdparty/boost:boost",
    ],
)

gen_fbthrift(
    name = "reflection",
    src = "thrift/lib/thrift/reflection.thrift",
    options = "templates,no_metadata,include_prefix=thrift/lib/thrift",
    output_path = "thrift/lib/thrift",
)

gen_fbthrift(
    name = "metadata",
    src = "thrift/lib/thrift/metadata.thrift",
    options = "include_prefix=thrift/lib/thrift",
    output_path = "thrift/lib/thrift",
    service_list = [
        "ThriftMetadataService",
    ],
)

gen_fbthrift(
    name = "frozen",
    src = "thrift/lib/thrift/frozen.thrift",
    options = "include_prefix=thrift/lib/thrift",
    output_path = "thrift/lib/thrift",
)

gen_fbthrift(
    name = "RpcMetadata",
    src = "thrift/lib/thrift/RpcMetadata.thrift",
    options = "no_metadata,include_prefix=thrift/lib/thrift",
    output_path = "thrift/lib/thrift",
)

cc_library(
    name = "thriftmetadata",
    srcs = [
        ":RpcMetadata",
        ":frozen",
        ":metadata",
    ],
    hdrs = glob([
        "thrift/lib/thrift/*.h",
        "thrift/lib/cpp2/**/*.h",
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        "@double-conversion//:double-conversion",
        "@folly",
        "@wangle",
    ],
)

cc_library(
    name = "thriftfrozen2",
    srcs = [
        "thrift/lib/cpp2/frozen/Frozen.cpp",
        "thrift/lib/cpp2/frozen/FrozenUtil.cpp",
        "thrift/lib/cpp2/frozen/schema/MemorySchema.cpp",
        ":frozen",
    ],
    hdrs = glob([
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":thriftmetadata",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "rpcmetadata",
    srcs = [
        "thrift/lib/cpp2/gen/module_types_cpp.cpp",
        ":RpcMetadata",
    ],
    hdrs = glob([
        "thrift/lib/cpp2/**/*.h",
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/thrift/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":thrift-core",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "thriftprotocol",
    srcs = [
        "thrift/lib/cpp2/protocol/BinaryProtocol.cpp",
        "thrift/lib/cpp2/protocol/CompactProtocol.cpp",
        "thrift/lib/cpp2/protocol/CompactV1Protocol.cpp",
        "thrift/lib/cpp2/protocol/DebugProtocol.cpp",
        "thrift/lib/cpp2/protocol/JSONProtocol.cpp",
        "thrift/lib/cpp2/protocol/JSONProtocolCommon.cpp",
        "thrift/lib/cpp2/protocol/Serializer.cpp",
        "thrift/lib/cpp2/protocol/VirtualProtocol.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp2/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":thrift",
        "@double-conversion//:double-conversion",
        "@folly",
        "@wangle",
    ],
)

cc_library(
    name = "thriftcpp2",
    srcs = [
        "thrift/lib/cpp2/Flags.cpp",
        "thrift/lib/cpp2/GeneratedCodeHelper.cpp",
        "thrift/lib/cpp2/async/AsyncClient.cpp",
        "thrift/lib/cpp2/async/AsyncProcessor.cpp",
        "thrift/lib/cpp2/async/ClientSinkBridge.cpp",
        "thrift/lib/cpp2/async/ClientStreamBridge.cpp",
        "thrift/lib/cpp2/async/Cpp2Channel.cpp",
        "thrift/lib/cpp2/async/DuplexChannel.cpp",
        "thrift/lib/cpp2/async/FramingHandler.cpp",
        "thrift/lib/cpp2/async/HeaderChannel.cpp",
        "thrift/lib/cpp2/async/HeaderChannelTrait.cpp",
        "thrift/lib/cpp2/async/HeaderClientChannel.cpp",
        "thrift/lib/cpp2/async/HeaderServerChannel.cpp",
        "thrift/lib/cpp2/async/RequestChannel.cpp",
        "thrift/lib/cpp2/async/ResponseChannel.cpp",
        "thrift/lib/cpp2/async/RocketClientChannel.cpp",
        "thrift/lib/cpp2/async/RpcTypes.cpp",
        "thrift/lib/cpp2/async/ServerGeneratorStream.cpp",
        "thrift/lib/cpp2/async/ServerSinkBridge.cpp",
        "thrift/lib/cpp2/security/extensions/ThriftParametersClientExtension.cpp",
        "thrift/lib/cpp2/security/extensions/ThriftParametersContext.cpp",
        "thrift/lib/cpp2/security/extensions/Types.cpp",
        "thrift/lib/cpp2/server/BaseThriftServer.cpp",
        "thrift/lib/cpp2/server/Cpp2ConnContext.cpp",
        "thrift/lib/cpp2/server/Cpp2Connection.cpp",
        "thrift/lib/cpp2/server/Cpp2Worker.cpp",
        "thrift/lib/cpp2/server/LoggingEvent.cpp",
        "thrift/lib/cpp2/server/RequestDebugLog.cpp",
        "thrift/lib/cpp2/server/RequestsRegistry.cpp",
        "thrift/lib/cpp2/server/ServerInstrumentation.cpp",
        "thrift/lib/cpp2/server/ThriftServer.cpp",
        "thrift/lib/cpp2/server/peeking/TLSHelper.cpp",
        "thrift/lib/cpp2/transport/core/RpcMetadataUtil.cpp",
        "thrift/lib/cpp2/transport/core/ThriftClient.cpp",
        "thrift/lib/cpp2/transport/core/ThriftClientCallback.cpp",
        "thrift/lib/cpp2/transport/core/ThriftProcessor.cpp",
        "thrift/lib/cpp2/transport/core/ThriftRequest.cpp",
        "thrift/lib/cpp2/transport/http2/client/H2ClientConnection.cpp",
        "thrift/lib/cpp2/transport/http2/client/ThriftTransactionHandler.cpp",
        "thrift/lib/cpp2/transport/http2/common/H2Channel.cpp",
        "thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.cpp",
        "thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.cpp",
        "thrift/lib/cpp2/transport/http2/server/ThriftRequestHandler.cpp",
        "thrift/lib/cpp2/transport/rocket/PayloadUtils.cpp",
        "thrift/lib/cpp2/transport/rocket/Types.cpp",
        "thrift/lib/cpp2/transport/rocket/client/RequestContext.cpp",
        "thrift/lib/cpp2/transport/rocket/client/RequestContextQueue.cpp",
        "thrift/lib/cpp2/transport/rocket/client/RocketClient.cpp",
        "thrift/lib/cpp2/transport/rocket/client/RocketStreamServerCallback.cpp",
        "thrift/lib/cpp2/transport/rocket/framing/ErrorCode.cpp",
        "thrift/lib/cpp2/transport/rocket/framing/Frames.cpp",
        "thrift/lib/cpp2/transport/rocket/framing/Parser.cpp",
        "thrift/lib/cpp2/transport/rocket/framing/Serializer.cpp",
        "thrift/lib/cpp2/transport/rocket/framing/Util.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketRoutingHandler.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.cpp",
        "thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.cpp",
        "thrift/lib/cpp2/transport/rocket/server/ThriftRocketServerHandler.cpp",
        "thrift/lib/cpp2/util/Checksum.cpp",
        "thrift/lib/cpp2/util/ScopedServerInterfaceThread.cpp",
        "thrift/lib/cpp2/util/ScopedServerThread.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp2/**/*.h",
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/transport/http2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":rpcmetadata",
        ":thrift",
        ":thriftfrozen2",
        ":thriftmetadata",
        ":thriftprotocol",
        "@//thirdparty/openssl:openssl",
        "@double-conversion//:double-conversion",
        "@folly",
        "@proxygen//:proxygenhttpserver",
        "@wangle",
    ],
)

cc_library(
    name = "thrift-core",
    srcs = [
        "thrift/lib/cpp/Thrift.cpp",
        "thrift/lib/cpp2/FieldRef.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        "@double-conversion//:double-conversion",
        "@fmt",
        "@folly",
    ],
)

cc_library(
    name = "concurrency",
    srcs = [
        "thrift/lib/cpp/concurrency/Monitor.cpp",
        "thrift/lib/cpp/concurrency/Mutex.cpp",
        "thrift/lib/cpp/concurrency/PosixThreadFactory.cpp",
        "thrift/lib/cpp/concurrency/ThreadManager.cpp",
        "thrift/lib/cpp/concurrency/TimerManager.cpp",
        "thrift/lib/cpp/concurrency/Util.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "protocol",
    srcs = [
        "thrift/lib/cpp/protocol/TBase64Utils.cpp",
        "thrift/lib/cpp/protocol/TDebugProtocol.cpp",
        "thrift/lib/cpp/protocol/TJSONProtocol.cpp",
        "thrift/lib/cpp/protocol/TProtocolException.cpp",
        "thrift/lib/cpp/protocol/TSimpleJSONProtocol.cpp",
        ":reflection",
    ],
    hdrs = glob([
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":thrift-core",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
        "@double-conversion//:double-conversion",
        "@fmt",
        "@folly",
    ],
)

cc_library(
    name = "transport",
    srcs = [
        "thrift/lib/cpp/transport/TBufferTransports.cpp",
        "thrift/lib/cpp/transport/TFDTransport.cpp",
        "thrift/lib/cpp/transport/THeader.cpp",
        "thrift/lib/cpp/transport/THttpClient.cpp",
        "thrift/lib/cpp/transport/THttpServer.cpp",
        "thrift/lib/cpp/transport/THttpTransport.cpp",
        "thrift/lib/cpp/transport/TSocket.cpp",
        "thrift/lib/cpp/transport/TTransportException.cpp",
        "thrift/lib/cpp/transport/TZlibTransport.cpp",
        "thrift/lib/cpp/util/PausableTimer.cpp",
        "thrift/lib/cpp/util/THttpParser.cpp",
        "thrift/lib/cpp/util/VarintUtils.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":concurrency",
        ":rpcmetadata",
        ":thrift-core",
        "@//thirdparty/openssl:openssl",
        "@//thirdparty/zlib:libz",
        "@//thirdparty/zstd:zstd",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "async",
    srcs = [
        "thrift/lib/cpp/ContextStack.cpp",
        "thrift/lib/cpp/EventHandlerBase.cpp",
        "thrift/lib/cpp/async/TBinaryAsyncChannel.cpp",
        "thrift/lib/cpp/async/TFramedAsyncChannel.cpp",
        "thrift/lib/cpp/async/THttpAsyncChannel.cpp",
        "thrift/lib/cpp/async/TUnframedAsyncChannel.cpp",
        "thrift/lib/cpp/async/TZlibAsyncChannel.cpp",
        "thrift/lib/cpp/server/TServerObserver.cpp",
    ],
    hdrs = glob([
        "thrift/lib/cpp/*.h",
        "thrift/lib/cpp/**/*.h",
        "thrift/lib/cpp2/*.h",
        "thrift/lib/cpp2/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    deps = [
        ":concurrency",
        ":transport",
        "@//thirdparty/boost:boost",
        "@//thirdparty/openssl:openssl",
        "@com_github_google_glog//:glog",
        "@double-conversion//:double-conversion",
        "@folly",
    ],
)

cc_library(
    name = "thrift",
    deps = [
        ":async",
        ":concurrency",
        ":protocol",
        ":transport",
        "@com_github_google_glog//:glog",
        "@folly",
    ],
)

#gen_fbthrift(
#  name = "chatroom",
#  src = "thrift/example/if/chatroom.thrift",
#  output_path = "thrift/example/if",
#  service_list = [
#    "ChatRoomService",
#    "Echo"
#  ],
#  options = "templates,no_metadata",
#)
#cc_binary(
#  name = "example_server",
#  srcs = [
#    "thrift/example/cpp2/server/EchoService.h",
#    "thrift/example/cpp2/server/EchoService.cpp",
#    "thrift/example/cpp2/server/ChatRoomService.h",
#    "thrift/example/cpp2/server/ChatRoomService.cpp",
#    "thrift/example/cpp2/server/ExampleServer.cpp",
#    ":chatroom",
#  ],
#  copts = [
#    "-Iexternal/double-conversion/",
#  ],
#  linkopts = [
#    '-ldl'
#  ],
#  deps = [
#    ":thriftcpp2",
#    "@com_github_google_glog//:glog",
#    "@com_github_gflags_gflags//:gflags",
#    "@//thirdparty/proxygen:proxygenhttpserver",
#    "@double-conversion//:double-conversion",
#    "@//thirdparty/openssl:openssl",
#  ]
#)

go_library(
    name = "fbthrift_go",
    srcs = [
        "thrift/lib/go/thrift/application_exception.go",
        "thrift/lib/go/thrift/binary_protocol.go",
        "thrift/lib/go/thrift/buffered_transport.go",
        "thrift/lib/go/thrift/client_interface.go",
        "thrift/lib/go/thrift/clientconn.go",
        "thrift/lib/go/thrift/compact_protocol.go",
        "thrift/lib/go/thrift/concurrent_server.go",
        "thrift/lib/go/thrift/context.go",
        "thrift/lib/go/thrift/debug_protocol.go",
        "thrift/lib/go/thrift/deserializer.go",
        "thrift/lib/go/thrift/exception.go",
        "thrift/lib/go/thrift/field.go",
        "thrift/lib/go/thrift/framed_transport.go",
        "thrift/lib/go/thrift/header.go",
        "thrift/lib/go/thrift/header_protocol.go",
        "thrift/lib/go/thrift/header_transport.go",
        "thrift/lib/go/thrift/http_client.go",
        "thrift/lib/go/thrift/http_transport.go",
        "thrift/lib/go/thrift/interceptor.go",
        "thrift/lib/go/thrift/iostream_transport.go",
        "thrift/lib/go/thrift/json_protocol.go",
        "thrift/lib/go/thrift/memory_buffer.go",
        "thrift/lib/go/thrift/messagetype.go",
        "thrift/lib/go/thrift/multiplexed_protocol.go",
        "thrift/lib/go/thrift/numeric.go",
        "thrift/lib/go/thrift/pointerize.go",
        "thrift/lib/go/thrift/processor.go",
        "thrift/lib/go/thrift/processor_factory.go",
        "thrift/lib/go/thrift/protocol.go",
        "thrift/lib/go/thrift/protocol_exception.go",
        "thrift/lib/go/thrift/protocol_factory.go",
        "thrift/lib/go/thrift/rich_transport.go",
        "thrift/lib/go/thrift/serializer.go",
        "thrift/lib/go/thrift/server.go",
        "thrift/lib/go/thrift/server_options.go",
        "thrift/lib/go/thrift/server_socket.go",
        "thrift/lib/go/thrift/server_transport.go",
        "thrift/lib/go/thrift/simple_json_protocol.go",
        "thrift/lib/go/thrift/simple_server.go",
        "thrift/lib/go/thrift/socket.go",
        "thrift/lib/go/thrift/ssl_server_socket.go",
        "thrift/lib/go/thrift/ssl_socket.go",
        "thrift/lib/go/thrift/transport.go",
        "thrift/lib/go/thrift/transport_exception.go",
        "thrift/lib/go/thrift/transport_factory.go",
        "thrift/lib/go/thrift/type.go",
        "thrift/lib/go/thrift/zlib_transport.go",
        "thrift/lib/go/thrift/zstd.go",
    ],
    importpath = "github.com/facebook/fbthrift/thrift/lib/go/thrift",
    visibility = ["//visibility:public"],
)
