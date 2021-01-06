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

load("@rules_jvm_external//:defs.bzl", "maven_install")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_jar")
load("@bazel_gazelle//:deps.bzl", "go_repository")

all_content = """filegroup(name = "all", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

def include_repositories():
    http_archive(
        name = "libevent",
        build_file_content = all_content,
        strip_prefix = "libevent-2.1.8-stable",
        urls = [
            "https://mirror.bazel.build/github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz",
            "https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz",
        ],
        sha256 = "965cc5a8bb46ce4199a47e9b2c9e1cae3b137e8356ffdad6d94d3b9069b71dc2",
    )

    http_archive(
        name = "zlib",
        build_file_content = all_content,
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
            "https://zlib.net/zlib-1.2.11.tar.gz",
        ],
    )

    http_archive(
        name = "liburing",
        build_file_content = all_content,
        strip_prefix = "liburing-liburing-0.7",
        urls = [
            "https://github.com/axboe/liburing/archive/liburing-0.7.tar.gz",
        ],
    )

    http_archive(
        name = "boost",
        build_file_content = all_content,
        strip_prefix = "boost_1_68_0",
        sha256 = "da3411ea45622579d419bfda66f45cd0f8c32a181d84adfa936f5688388995cf",
        urls = [
            "https://mirror.bazel.build/dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz",
            "https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.gz",
        ],
    )

    http_archive(
        name = "bison",
        build_file_content = all_content,
        strip_prefix = "bison-3.6.2",
        sha256 = "e28ed3aad934de2d1df68be209ac0b454f7b6d3c3d6d01126e5cd2cbadba089a",
        urls = ["https://ftp.gnu.org/gnu/bison/bison-3.6.2.tar.gz"],
    )

    http_archive(
        name = "flex",
        build_file_content = all_content,
        strip_prefix = "flex-2.6.4",
        sha256 = "e87aae032bf07c26f85ac0ed3250998c37621d95f8bd748b31f15b33c45ee995",
        urls = ["https://github.com/westes/flex/releases/download/v2.6.4/flex-2.6.4.tar.gz"],
        patches = ["@//thirdparty/tools:flex.patch"],
    )

    http_archive(
        name = "m4",
        build_file_content = all_content,
        strip_prefix = "m4-1.4.18",
        sha256 = "ab2633921a5cd38e48797bf5521ad259bdc4b979078034a3b790d7fec5493fab",
        urls = ["https://ftp.gnu.org/gnu/m4/m4-1.4.18.tar.gz"],
        patches = ["@//thirdparty/tools:m4.patch"],
    )

    http_archive(
        name = "openssl",
        build_file_content = all_content,
        sha256 = "f6fb3079ad15076154eda9413fed42877d668e7069d9b87396d0804fdb3f4c90",
        strip_prefix = "openssl-1.1.1c",
        urls = ["https://www.openssl.org/source/openssl-1.1.1c.tar.gz"],
    )

    http_archive(
        name = "libunwind",
        build_file_content = all_content,
        strip_prefix = "libunwind-9165d2a150d707d3037c2045f2cdc0fabd5fee98",
        urls = [
            "https://mirror.bazel.build/github.com/libunwind/libunwind/archive/9165d2a150d707d3037c2045f2cdc0fabd5fee98.zip",
            "https://github.com/libunwind/libunwind/archive/9165d2a150d707d3037c2045f2cdc0fabd5fee98.zip",
        ],
        sha256 = "f83c604cde80a49af91345a1ff3f4558958202989fb768e6508963e24ea2524c",
    )

    http_archive(
        name = "libsodium",
        build_file_content = all_content,
        strip_prefix = "libsodium-1.0.18",
        urls = [
            "https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz",
        ],
        sha256 = "6f504490b342a4f8a4c4a02fc9b866cbef8622d5df4e5452b46be121e46636c1",
    )

    http_archive(
        name = "lz4",
        build_file_content = all_content,
        strip_prefix = "lz4-1.9.2",
        urls = [
            "https://github.com/lz4/lz4/archive/v1.9.2.tar.gz",
        ],
        sha256 = "658ba6191fa44c92280d4aa2c271b0f4fbc0e34d249578dd05e50e76d0e5efcc",
    )

    http_archive(
        name = "zstd",
        build_file_content = all_content,
        strip_prefix = "zstd-1.4.5",
        urls = [
            "https://github.com/facebook/zstd/releases/download/v1.4.5/zstd-1.4.5.tar.gz",
        ],
        sha256 = "98e91c7c6bf162bf90e4e70fdbc41a8188b9fa8de5ad840c401198014406ce9e",
    )

    http_archive(
        name = "bzip2",
        build_file_content = all_content,
        strip_prefix = "bzip2-1.0.8",
        urls = [
            "https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz",
        ],
        sha256 = "ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269",
    )

    http_archive(
        name = "snappy",
        build_file = "@//thirdparty/snappy:snappy.BUILD",
        patches = [
            "@//thirdparty/snappy:snappy.patch",
        ],
        strip_prefix = "snappy-1.1.8",
        urls = [
            "https://github.com/google/snappy/archive/1.1.8.tar.gz",
        ],
        sha256 = "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f",
    )

    http_archive(
        name = "double-conversion",
        build_file = "@//thirdparty/double-conversion:double-conversion.BUILD",
        strip_prefix = "double-conversion-3.1.5",
        urls = [
            "https://github.com/google/double-conversion/archive/v3.1.5.tar.gz",
        ],
        sha256 = "a63ecb93182134ba4293fd5f22d6e08ca417caafa244afaa751cbfddf6415b13",
    )

    http_archive(
        name = "folly",
        build_file = "@//thirdparty/folly:folly.BUILD",
        patches = [
            "@//thirdparty/folly:folly.patch",
        ],
        urls = [
            "https://github.com/facebook/folly/releases/download/v2020.10.26.00/folly-v2020.10.26.00.tar.gz",
        ],
        sha256 = "50faf3973f23660adac85e8f545f82be37bd6cdf6b768fd24d47aaaa9816ae7a",
    )

    http_archive(
        name = "fmt",
        build_file = "@//thirdparty/fmt:fmt.BUILD",
        strip_prefix = "fmt-7.1.0",
        urls = [
            "https://github.com/fmtlib/fmt/archive/7.1.0.tar.gz",
        ],
        sha256 = "a53bce7e3b7ee8c7374723262a43356afff176b1684b86061748409e6f8b56c5",
    )

    http_archive(
        name = "com_github_gflags_gflags",
        strip_prefix = "gflags-2.2.2",
        urls = [
            "https://mirror.bazel.build/github.com/gflags/gflags/archive/v2.2.2.tar.gz",
            "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
        ],
        sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    )

    http_archive(
        name = "com_github_google_glog",
        strip_prefix = "glog-0.4.0",
        urls = [
            "https://github.com/google/glog/archive/v0.4.0.tar.gz",
        ],
        sha256 = "f28359aeba12f30d73d9e4711ef356dc842886968112162bc73002645139c39c",
    )

    http_archive(
        name = "com_github_google_gtest",
        strip_prefix = "googletest-release-1.10.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
        ],
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
    )

    http_archive(
        name = "fizz",
        build_file = "@//thirdparty/fizz:fizz.BUILD",
        urls = [
            "https://github.com/facebookincubator/fizz/releases/download/v2020.10.26.00/fizz-v2020.10.26.00.tar.gz",
        ],
        sha256 = "e4803b5cedd1dc2b51963c5e75f4a17dfe872e96e67dcf62948297db90dc7d78",
    )

    http_archive(
        name = "wangle",
        build_file = "@//thirdparty/wangle:wangle.BUILD",
        urls = [
            "https://github.com/facebook/wangle/releases/download/v2020.10.26.00/wangle-v2020.10.26.00.tar.gz",
        ],
        sha256 = "bfe7c6ff0856cb25fb841c79399c5263c9a7b3161f62c000ffa552a4aa48b1eb",
    )

    http_archive(
        name = "proxygen",
        build_file = "@//thirdparty/proxygen:proxygen.BUILD",
        urls = [
            "https://github.com/facebook/proxygen/releases/download/v2020.10.26.00/proxygen-v2020.10.26.00.tar.gz",
        ],
        sha256 = "b39809c8fbf1e8bdd9928be612e8c8bcb85344fc95172433c045c54e05d23765",
    )

    http_archive(
        name = "fbthrift",
        build_file = "@//thirdparty/fbthrift:fbthrift.BUILD",
        strip_prefix = "fbthrift-2020.10.26.00",
        patches = [
            "@//thirdparty/fbthrift:0001-fixed-thrift-sync-wait-fatal-error.patch",
        ],
        urls = [
            "https://github.com/facebook/fbthrift/archive/v2020.10.26.00.tar.gz",
        ],
        sha256 = "31427f39d1c0a01e87981d18d22425e98f89a23f6676ad2f74075dafd347d81a",
    )

    http_archive(
        name = "cityhash",
        build_file = "@//thirdparty/cityhash:cityhash.BUILD",
        strip_prefix = "cityhash-master",
        patches = [
            "@//thirdparty/cityhash:cityhash.patch",
        ],
        urls = [
            "https://github.com/google/cityhash/archive/master.zip",
        ],
        sha256 = "2d3d236b799a7298827ec37ee21d8b0d9190e15add6f02ae21d651d812747152",
    )

    http_archive(
        name = "rocksdb",
        build_file = "@//thirdparty/rocksdb:rocksdb.BUILD",
        strip_prefix = "rocksdb-6.12.7",
        patches = [
            "@//thirdparty/rocksdb:0001-patch-rocksdb.patch",
        ],
        urls = [
            "https://github.com/facebook/rocksdb/archive/v6.12.7.tar.gz",
        ],
        sha256 = "fee38528108b6b49f813b9f055584b123ff0debbc3f39584d4a663c46075a6e9",
    )

    http_archive(
        name = "wdt",
        build_file = "@//thirdparty/wdt:wdt.BUILD",
        patch_cmds = [
            "sed -i 's/<wdt\\//</g' `grep -rl '<wdt/'`",
        ],
        strip_prefix = "wdt-27e66118bfbbac685b2ceb11e09c45209f9a510c",
        urls = [
            "https://github.com/facebook/wdt/archive/27e66118bfbbac685b2ceb11e09c45209f9a510c.zip",
        ],
        sha256 = "b24f7e5424caf8443d42272c67eff9f8fb0f99a3716a56f91bc6f0c738f15b84",
    )

    http_jar(
        name = "lombok",
        url = "https://projectlombok.org/downloads/lombok.jar",
        sha256 = "7206cbbfd6efd5e85bceff29545633645650be58d58910a23b0d4835fbd15ed7",
    )

    maven_install(
        artifacts = [
            "junit:junit:4.12",
            "org.slf4j:slf4j-api:1.7.30",
            "org.apache.hadoop:hadoop-common:2.7.3",
            "org.apache.hadoop:hadoop-hdfs:2.7.3",
            "org.apache.hadoop:hadoop-mapreduce-client-core:2.7.3",
            "org.apache.hadoop:hadoop-mapreduce-client-jobclient:2.7.3",
            "org.xerial.snappy:snappy-java:1.1.7.3",
            "commons-codec:commons-codec:1.15",
            "commons-collections:commons-collections:3.2.2",
            "log4j:log4j:1.2.17",
        ],
        repositories = [
            "https://maven.aliyun.com/repository/central",
            # "https://repo1.maven.org/maven2",
        ],
    )

    go_repository(
        name = "com_github_rcrowley_go_metrics",
        importpath = "github.com/rcrowley/go-metrics",
        commit = "cf1acfcdf4751e0554ffa765d03e479ec491cad6",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_m3db_prometheus_client_golang",
        importpath = "github.com/m3db/prometheus_client_golang",
        sum = "h1:t7w/tcFws81JL1j5sqmpqcOyQOpH4RDOmIe3A3fdN3w=",
        version = "v0.8.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_m3db_prometheus_client_model",
        importpath = "github.com/m3db/prometheus_client_model",
        sum = "h1:cg1+DiuyT6x8h9voibtarkH1KT6CmsewBSaBhe8wzLo=",
        version = "v0.1.0",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_m3db_client_golang",
        importpath = "github.com/m3db/client_golang",
        sum = "h1:scioj4kTt1vUeBlW6fZVjh3L30FEZ1m7K7V3PszXSyM=",
        version = "v0.8.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_m3db_prometheus_procfs",
        importpath = "github.com/m3db/prometheus_procfs",
        sum = "h1:LsxWzVELhDU9sLsZTaFLCeAwCn7bC7qecZcK4zobs/g=",
        version = "v0.8.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_m3db_prometheus_common",
        importpath = "github.com/m3db/prometheus_common",
        sum = "h1:YJu6eCIV6MQlcwND24cRG/aRkZDX1jvYbsNNs1ZYr0w=",
        version = "v0.1.0",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_matttproud_golang_protobuf_extensions",
        importpath = "github.com/matttproud/golang_protobuf_extensions",
        sum = "h1:4hp9jkHxhMHkqkrB3Ix0jegS5sx/RkqARlsWZ6pIwiU=",
        version = "v1.0.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_beorn7_perks",
        importpath = "github.com/beorn7/perks",
        sum = "h1:VlbKKnNfV8bJzeqoa4cOKqO6bYr3WgKZxO8Z16+hsOM=",
        version = "v1.0.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_liubang_tally",
        importpath = "github.com/liubang/tally",
        commit = "952b6f95b269e6c2d5d5e6f3f56ea8234e82b880",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_zentures_cityhash",
        importpath = "github.com/zentures/cityhash",
        commit = "cdd6a94144ab18dd21ad865a3aa6837585bffa15",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_hashicorp_consul",
        importpath = "github.com/hashicorp/consul",
        tag = "v1.9.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "org_uber_go_atomic",
        importpath = "go.uber.org/atomic",
        sum = "h1:rsqfU5vBkVknbhUGbAUwQKR2H4ItV8tjJ+6kJX4cxHM=",
        version = "v1.5.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_stretchr_testify",
        importpath = "github.com/stretchr/testify",
        sum = "h1:2E4SXV/wtOkTonXsotYi4li6zVWxYlZuYNCXe9XRJyk=",
        version = "v1.4.0",
        build_file_generation = "on",
    )

    go_repository(
        name = "com_github_davecgh_go_spew",
        importpath = "github.com/davecgh/go-spew",
        sum = "h1:vj9j/u1bqnvCEfJOwUhtlOARqs3+rkHYY13jYWTU97c=",
        version = "v1.1.1",
        build_file_generation = "on",
    )

    go_repository(
        name = "in_gopkg_yaml_v2",
        importpath = "gopkg.in/yaml.v2",
        sum = "h1:VUgggvou5XRW9mHwD/yXxIYSMtY0zoKQf/v226p2nyo=",
        version = "v2.2.7",
    )

def external_maven_jars():
    http_archive(
        name = "bazel_springboot_rule",
        sha256 = "2740456f3d7eb8400b9832a44d60df1b0ecf0ff3e8b3086faa029e4df2d8ac88",
        urls = [
            "https://github.com/salesforce/bazel-springboot-rule/releases/download/1.0.8/bazel-springboot-rule-1.0.8.zip",
        ],
    )
    maven_install(
        excluded_artifacts = [
            "org.slf4j:slf4j-log4j12",
            "com.google.code.gson:gson",
            "org.springframework.boot:spring-boot-starter-logging",
        ],
        artifacts = [
            "org.springframework.boot:spring-boot-loader:2.4.1",
            "org.springframework.boot:spring-boot-actuator:2.4.1",
            "org.springframework.boot:spring-boot-autoconfigure:2.4.1",
            "org.springframework.boot:spring-boot-starter-web:2.4.1",
            "org.springframework.boot:spring-boot-starter-log4j2:2.4.1",
            "org.springframework.boot:spring-boot-starter-test:2.4.1",
            "org.apache.hadoop:hadoop-hdfs:2.7.3",
            "org.apache.hadoop:hadoop-common:2.7.3",
            "org.apache.hadoop:hadoop-client:2.7.3",
            "junit:junit:4.12",
            "com.alibaba:fastjson:1.2.18",
            "com.squareup.okhttp3:okhttp:3.14.6",
            "com.fasterxml.jackson.core:jackson-annotations:2.12.0",
            "org.springframework.cloud:spring-cloud-context:3.0.0",
            "org.slf4j:slf4j-api:1.7.30",
            "commons-lang:commons-lang:2.6",
            "commons-codec:commons-codec:1.6",
            "jakarta.annotation:jakarta.annotation-api:1.3.5",
        ],
        fetch_sources = True,
        repositories = [
            "http://maven.aliyun.com/nexus/content/groups/public/",
        ],
    )
