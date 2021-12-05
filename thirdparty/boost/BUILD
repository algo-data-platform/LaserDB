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

load("@rules_foreign_cc//tools/build_defs:boost_build.bzl", "boost_build")

boost_build(
    name = "boost",
    bootstrap_options = ["--with-libraries=boost_context,boost_chrono,boost_date_time,boost_program_options,boost_filesystem,boost_regex,boost_system,boost_thread,boost_random"],
    lib_source = "@boost//:all",
    static_libraries = [
        "libboost_chrono.a",
        "libboost_context.a",
        "libboost_date_time.a",
        "libboost_filesystem.a",
        "libboost_program_options.a",
        "libboost_random.a",
        "libboost_system.a",
        "libboost_thread.a",
        "libboost_regex.a",
    ],
    user_options = [
        "--with-context",
        "--with-chrono",
        "--with-date_time",
        "--with-program_options",
        "--with-filesystem",
        "--with-regex",
        "--with-system",
        "--with-thread",
        "--with-random",
    ],
    visibility = ["//visibility:public"],
)
