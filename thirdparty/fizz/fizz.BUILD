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
      "fizz/client/*.h",
      "fizz/crypto/*.h",
      "fizz/crypto/aead/*.h",
      "fizz/crypto/exchange/*.h",
      "fizz/crypto/signature/*.h",
      "fizz/crypto/openssl/*.h",
      "fizz/extensions/delegatedcred/*.h",
      "fizz/extensions/exportedauth/*.h",
      "fizz/extensions/tokenbinding/*.h",
      "fizz/experimental/**/*.h",
      "fizz/protocol/*.h",
      "fizz/protocol/clock/*.h",
      "fizz/record/*.h",
      "fizz/server/*.h",
      "fizz/util/*.h",
      "fizz/tool/*.h",
    ], 
  ),
)

cc_library(
  name = "fizz",
  srcs = [
    "fizz/crypto/Utils.cpp",
    "fizz/crypto/exchange/X25519.cpp",
    "fizz/crypto/aead/OpenSSLEVPCipher.cpp",
    "fizz/crypto/aead/IOBufUtil.cpp",
    "fizz/crypto/signature/Signature.cpp",
    "fizz/crypto/Hkdf.cpp",
    "fizz/crypto/KeyDerivation.cpp",
    "fizz/crypto/Sha256.cpp",
    "fizz/crypto/Sha384.cpp",
    "fizz/crypto/openssl/OpenSSLKeyUtils.cpp",
    "fizz/record/Types.cpp",
    "fizz/record/RecordLayer.cpp",
    "fizz/record/EncryptedRecordLayer.cpp",
    "fizz/record/PlaintextRecordLayer.cpp",
    "fizz/server/AeadTokenCipher.cpp",
    "fizz/server/AeadCookieCipher.cpp",
    "fizz/server/ServerProtocol.cpp",
    "fizz/server/CertManager.cpp",
    "fizz/server/State.cpp",
    "fizz/server/FizzServer.cpp",
    "fizz/server/TicketCodec.cpp",
    "fizz/server/CookieCipher.cpp",
    "fizz/server/ReplayCache.cpp",
    "fizz/server/SlidingBloomReplayCache.cpp",
    "fizz/protocol/AsyncFizzBase.cpp",
    "fizz/protocol/Types.cpp",
    "fizz/protocol/Exporter.cpp",
    "fizz/protocol/DefaultCertificateVerifier.cpp",
    "fizz/protocol/Events.cpp",
    "fizz/protocol/KeyScheduler.cpp",
    "fizz/protocol/Certificate.cpp",
    "fizz/protocol/CertDecompressionManager.cpp",
    "fizz/protocol/Params.cpp",
    "fizz/protocol/ZlibCertificateCompressor.cpp",
    "fizz/protocol/ZlibCertificateDecompressor.cpp",
    "fizz/protocol/clock/SystemClock.cpp",
    "fizz/extensions/delegatedcred/DelegatedCredentialCertManager.cpp",
    "fizz/extensions/delegatedcred/DelegatedCredentialClientExtension.cpp",
    "fizz/extensions/delegatedcred/DelegatedCredentialFactory.cpp",
    "fizz/extensions/delegatedcred/DelegatedCredentialUtils.cpp",
    "fizz/extensions/delegatedcred/Types.cpp",
    "fizz/extensions/exportedauth/ExportedAuthenticator.cpp",
    "fizz/extensions/tokenbinding/Types.cpp",
    "fizz/extensions/tokenbinding/TokenBindingConstructor.cpp",
    "fizz/extensions/tokenbinding/TokenBindingClientExtension.cpp",
    "fizz/extensions/tokenbinding/Validator.cpp",
    "fizz/experimental/client/BatchSignaturePeerCert.cpp",
    "fizz/experimental/protocol/BatchSignatureTypes.cpp",
    "fizz/client/State.cpp",
    "fizz/client/ClientProtocol.cpp",
    "fizz/client/PskSerializationUtils.cpp",
    "fizz/client/SynchronizedLruPskCache.cpp",
    "fizz/client/EarlyDataRejectionPolicy.cpp",
    "fizz/util/FizzUtil.cpp",
  ],
  hdrs = [
    ":header_files"
  ],
  includes = [
    "."
  ],
  deps = [
    "@folly//:folly",
    "@//thirdparty/libevent:libevent",
    "@//thirdparty/libsodium:libsodium",
    "@double-conversion//:double-conversion",
    "@com_github_google_glog//:glog",
    "@com_github_gflags_gflags//:gflags",
    "@//thirdparty/openssl:openssl",
  ],
  visibility = ["//visibility:public"],
  copts = [
    "-Iexternal/double-conversion/",
  ]
)
