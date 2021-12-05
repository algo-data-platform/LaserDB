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

genrule(
    name = "gen_http_common_headers",
    srcs = [
        "proxygen/lib/utils/perfect_hash_table_template.h",
        "proxygen/lib/utils/perfect_hash_table_template.cpp.gperf",
        "proxygen/lib/http/HTTPCommonHeaders.txt",
    ],
    outs = [
        "proxygen/lib/http/HTTPCommonHeaders.h",
        "proxygen/lib/http/HTTPCommonHeaders.cpp",
    ],
    cmd = " ".join([
        "dst=$(location proxygen/lib/http/HTTPCommonHeaders.cpp) &&",
        "dst=$${dst%/*} &&",
        "$(location proxygen/lib/http/gen_HTTPCommonHeaders.sh)",
        "$(location proxygen/lib/http/HTTPCommonHeaders.txt)",
        "external/proxygen",
        "$${dst}",
    ]),
    tools = [
        "proxygen/lib/http/gen_HTTPCommonHeaders.sh",
        "proxygen/lib/utils/gen_perfect_hash_table.sh",
    ],
)

genrule(
    name = "gen_trace_event_constants",
    srcs = [
        "proxygen/lib/utils/gen_trace_event_constants.py",
        "proxygen/lib/utils/samples/TraceFieldType.txt",
        "proxygen/lib/utils/samples/TraceEventType.txt",
    ],
    outs = [
        "proxygen/lib/utils/TraceEventType.h",
        "proxygen/lib/utils/TraceEventType.cpp",
        "proxygen/lib/utils/TraceFieldType.h",
        "proxygen/lib/utils/TraceFieldType.cpp",
    ],
    cmd = "python $(location proxygen/lib/utils/gen_trace_event_constants.py) --output_type=cpp --input_files=$(location proxygen/lib/utils/samples/TraceFieldType.txt),$(location proxygen/lib/utils/samples/TraceEventType.txt) --output_scope=proxygen --header_path=proxygen/lib/utils --install_dir=$(@D)/proxygen/lib/utils --fbcode_dir=`dirname $(location proxygen/lib/utils/gen_trace_event_constants.py)`/../../../",
)

cc_library(
    name = "proxygen",
    srcs = [
        "proxygen/external/http_parser/http_parser_cpp.cpp",
        "proxygen/lib/healthcheck/ServerHealthCheckerCallback.cpp",
        "proxygen/lib/http/HTTPConnector.cpp",
        "proxygen/lib/http/HTTPConnectorWithFizz.cpp",
        "proxygen/lib/http/HTTPConstants.cpp",
        "proxygen/lib/http/HTTPException.cpp",
        "proxygen/lib/http/HTTPHeaders.cpp",
        "proxygen/lib/http/HTTPMessage.cpp",
        "proxygen/lib/http/HTTPMessageFilters.cpp",
        "proxygen/lib/http/HTTPMethod.cpp",
        "proxygen/lib/http/ProxyStatus.cpp",
        "proxygen/lib/http/ProxygenErrorEnum.cpp",
        "proxygen/lib/http/RFC2616.cpp",
        "proxygen/lib/http/StatusTypeEnum.cpp",
        "proxygen/lib/http/Window.cpp",
        "proxygen/lib/http/codec/CodecProtocol.cpp",
        "proxygen/lib/http/codec/CodecUtil.cpp",
        "proxygen/lib/http/codec/DefaultHTTPCodecFactory.cpp",
        "proxygen/lib/http/codec/ErrorCode.cpp",
        "proxygen/lib/http/codec/FlowControlFilter.cpp",
        "proxygen/lib/http/codec/HTTP1xCodec.cpp",
        "proxygen/lib/http/codec/HTTP2Codec.cpp",
        "proxygen/lib/http/codec/HTTP2Constants.cpp",
        "proxygen/lib/http/codec/HTTP2Framer.cpp",
        "proxygen/lib/http/codec/HTTPChecks.cpp",
        "proxygen/lib/http/codec/HTTPCodecFactory.cpp",
        "proxygen/lib/http/codec/HTTPCodecFilter.cpp",
        "proxygen/lib/http/codec/HTTPCodecPrinter.cpp",
        "proxygen/lib/http/codec/HTTPParallelCodec.cpp",
        "proxygen/lib/http/codec/HTTPSettings.cpp",
        "proxygen/lib/http/codec/HeaderConstants.cpp",
        "proxygen/lib/http/codec/HeaderDecodeInfo.cpp",
        "proxygen/lib/http/codec/SPDYCodec.cpp",
        "proxygen/lib/http/codec/SPDYConstants.cpp",
        "proxygen/lib/http/codec/TransportDirection.cpp",
        "proxygen/lib/http/codec/compress/GzipHeaderCodec.cpp",
        "proxygen/lib/http/codec/compress/HPACKCodec.cpp",
        "proxygen/lib/http/codec/compress/HPACKContext.cpp",
        "proxygen/lib/http/codec/compress/HPACKDecodeBuffer.cpp",
        "proxygen/lib/http/codec/compress/HPACKDecoder.cpp",
        "proxygen/lib/http/codec/compress/HPACKDecoderBase.cpp",
        "proxygen/lib/http/codec/compress/HPACKEncodeBuffer.cpp",
        "proxygen/lib/http/codec/compress/HPACKEncoder.cpp",
        "proxygen/lib/http/codec/compress/HPACKEncoderBase.cpp",
        "proxygen/lib/http/codec/compress/HPACKHeader.cpp",
        "proxygen/lib/http/codec/compress/HeaderIndexingStrategy.cpp",
        "proxygen/lib/http/codec/compress/HeaderTable.cpp",
        "proxygen/lib/http/codec/compress/Huffman.cpp",
        "proxygen/lib/http/codec/compress/Logging.cpp",
        "proxygen/lib/http/codec/compress/NoPathIndexingStrategy.cpp",
        "proxygen/lib/http/codec/compress/QPACKCodec.cpp",
        "proxygen/lib/http/codec/compress/QPACKContext.cpp",
        "proxygen/lib/http/codec/compress/QPACKDecoder.cpp",
        "proxygen/lib/http/codec/compress/QPACKEncoder.cpp",
        "proxygen/lib/http/codec/compress/QPACKHeaderTable.cpp",
        "proxygen/lib/http/codec/compress/QPACKStaticHeaderTable.cpp",
        "proxygen/lib/http/codec/compress/StaticHeaderTable.cpp",
        "proxygen/lib/http/connpool/ServerIdleSessionController.cpp",
        "proxygen/lib/http/connpool/SessionHolder.cpp",
        "proxygen/lib/http/connpool/SessionPool.cpp",
        "proxygen/lib/http/connpool/ThreadIdleSessionController.cpp",
        "proxygen/lib/http/experimental/RFC1867.cpp",
        "proxygen/lib/http/session/ByteEventTracker.cpp",
        "proxygen/lib/http/session/ByteEvents.cpp",
        "proxygen/lib/http/session/CodecErrorResponseHandler.cpp",
        "proxygen/lib/http/session/HTTP2PriorityQueue.cpp",
        "proxygen/lib/http/session/HTTPDefaultSessionCodecFactory.cpp",
        "proxygen/lib/http/session/HTTPDirectResponseHandler.cpp",
        "proxygen/lib/http/session/HTTPDownstreamSession.cpp",
        "proxygen/lib/http/session/HTTPErrorPage.cpp",
        "proxygen/lib/http/session/HTTPEvent.cpp",
        "proxygen/lib/http/session/HTTPSession.cpp",
        "proxygen/lib/http/session/HTTPSessionAcceptor.cpp",
        "proxygen/lib/http/session/HTTPSessionBase.cpp",
        "proxygen/lib/http/session/HTTPTransaction.cpp",
        "proxygen/lib/http/session/HTTPTransactionEgressSM.cpp",
        "proxygen/lib/http/session/HTTPTransactionIngressSM.cpp",
        "proxygen/lib/http/session/HTTPUpstreamSession.cpp",
        "proxygen/lib/http/session/SecondaryAuthManager.cpp",
        "proxygen/lib/http/session/SimpleController.cpp",
        "proxygen/lib/http/structuredheaders/StructuredHeadersBuffer.cpp",
        "proxygen/lib/http/structuredheaders/StructuredHeadersDecoder.cpp",
        "proxygen/lib/http/structuredheaders/StructuredHeadersEncoder.cpp",
        "proxygen/lib/http/structuredheaders/StructuredHeadersUtilities.cpp",
        "proxygen/lib/pools/generators/FileServerListGenerator.cpp",
        "proxygen/lib/pools/generators/ServerListGenerator.cpp",
        "proxygen/lib/services/RequestWorkerThread.cpp",
        "proxygen/lib/services/Service.cpp",
        "proxygen/lib/services/WorkerThread.cpp",
        "proxygen/lib/stats/ResourceStats.cpp",
        "proxygen/lib/transport/PersistentFizzPskCache.cpp",
        "proxygen/lib/utils/AsyncTimeoutSet.cpp",
        "proxygen/lib/utils/Base64.cpp",
        "proxygen/lib/utils/CryptUtil.cpp",
        "proxygen/lib/utils/Exception.cpp",
        "proxygen/lib/utils/HTTPTime.cpp",
        "proxygen/lib/utils/Logging.cpp",
        "proxygen/lib/utils/ParseURL.cpp",
        "proxygen/lib/utils/RendezvousHash.cpp",
        "proxygen/lib/utils/Time.cpp",
        "proxygen/lib/utils/TraceEvent.cpp",
        "proxygen/lib/utils/TraceEventContext.cpp",
        "proxygen/lib/utils/WheelTimerInstance.cpp",
        "proxygen/lib/utils/ZlibStreamCompressor.cpp",
        "proxygen/lib/utils/ZlibStreamDecompressor.cpp",
        "proxygen/lib/utils/ZstdStreamCompressor.cpp",
        "proxygen/lib/utils/ZstdStreamDecompressor.cpp",
        ":gen_http_common_headers",
        ":gen_trace_event_constants",
    ],
    hdrs = glob([
        "proxygen/lib/http/**/*.h",
        "proxygen/lib/utils/**/*.h",
        "proxygen/lib/**/*.h",
        "proxygen/external/http_parser/http_parser.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@double-conversion//:double-conversion",
        "@fizz",
        "@folly",
        "@wangle",
    ],
)

cc_library(
    name = "proxygenhttpserver",
    srcs = [
        "proxygen/httpserver/HTTPServer.cpp",
        "proxygen/httpserver/HTTPServerAcceptor.cpp",
        "proxygen/httpserver/RequestHandlerAdaptor.cpp",
        "proxygen/httpserver/SignalHandler.cpp",
    ],
    hdrs = glob([
        "proxygen/httpserver/*.h",
        "proxygen/httpserver/**/*.h",
    ]),
    copts = [
        "-Iexternal/double-conversion/",
    ],
    includes = [
        ".",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":proxygen",
        "@double-conversion//:double-conversion",
        "@fizz",
        "@folly",
        "@wangle",
    ],
)
