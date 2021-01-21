/*
 * Copyright (c) 2020-present, Weibo, Inc.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author ZhongXiu Hao <nmred.hao@gmail.com>
 */

#pragma once

#include <vector>
#include "folly/Conv.h"
#include "folly/Optional.h"
#include "folly/String.h"

namespace hdfs {

enum class ResultStatus {
  NONE = -3,
  RUN_FAIL = -1,
  RUN_SUCC = 0,
};
// HdfsCount 用来存放count命令产生的输出，形式为目录数/文件数/字符数三种类型
struct HdfsCount {
  uint32_t dirs = 0;
  uint32_t files = 0;
  uint32_t words = 0;
};
// HdfsDf 用来存放df命令产生的输出形式为:文件/ size / used / free
struct HdfsDf {
  std::string filesystem;
  uint64_t total_capacity;
  uint64_t used_capacity;
  uint64_t free_capacity;
};

enum class FileType {
  FILE = 0,
  DIRECTORY = 1
};

struct HdfsLs {
  uint64_t file_words;
  std::string file_generated_time;
  std::string file_name;
  FileType type;
};

class HdfsCmd {
 public:
  HdfsCmd() = default;
  ~HdfsCmd() = default;
  // hadoop fs -get [-ignorecrc] [-crc] [-p] [-f] <src> <localdst>
  ResultStatus get(const std::string &src, const std::string &dest,
                   const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -cp [-f] [-p | -p[topax]] URI [URI ...] <dest>
  ResultStatus cp(const std::string &src, const std::string &dest,
                  const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -put [-f] [-p] [-l] [-d] [ - | <localsrc1> .. ]. <dst>
  ResultStatus put(const std::string &src, const std::string &dest,
                   const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -mv URI [URI ...] <dest>
  ResultStatus mv(const std::string &src, const std::string &dest,
                  const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -appendToFile <localsrc> ... <dst>
  ResultStatus appendToFile(const std::string &src, const std::string &dest);
  // hadoop fs -cat [-ignoreCrc] URI [URI ...]
  ResultStatus cat(std::string *result, const std::string &path, const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -checksum URI
  ResultStatus checksum(std::vector<std::string> *checksum, const std::string &path);
  // hadoop fs -chgrp [-R] GROUP URI [URI ...]
  ResultStatus chgrp(const std::string &paths, const std::string &group_name,
                     const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -chmod [-R] <MODE[,MODE]... | OCTALMODE> URI [URI ...]
  ResultStatus chmod(const std::string &paths, const folly::Optional<std::string> &option = folly::none);
  // hadoop fs -chown [-R] [OWNER][:[GROUP]] URI [URI ]
  ResultStatus chown(const std::string &paths, const std::string &group_user,
                     const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -copyFromLocal <localsrc> URI
  ResultStatus copyFromLocal(const std::string &src, const std::string &dest,
                             const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -copyToLocal [-ignorecrc] [-crc] URI <localdst>
  ResultStatus copyToLocal(const std::string &src, const std::string &dest,
                           const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -count [-q] [-h] [-v] [-x] [-t [<storage type>]] [-u] <paths>
  ResultStatus count(HdfsCount *ret, const std::string &paths, const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -df [-h] URI [URI ...]
  ResultStatus df(std::vector<HdfsDf> *output, const std::string &paths,
                  const folly::Optional<std::string> &optional = folly::none);
  // hadoop fshdfscount ret URI [URI ...]
  ResultStatus du(std::vector<std::pair<uint64_t, std::string>> *output, const std::string &paths,
                  const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -getmerge [-nl] <src> <localdst>
  ResultStatus getmerge(const std::string &src, const std::string &dest,
                        const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -ls [-C] [-d] [-h] [-q] [-R] [-t] [-S] [-r] [-u] <args>
  ResultStatus ls(std::vector<HdfsLs> *output, const std::string &path,
                  const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -mkdir [-p] <paths>
  ResultStatus mkdir(const std::string &paths, const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -moveFromLocal <localsrc> <dst>
  ResultStatus moveFromLocal(const std::string &src, const std::string &dest);
  // hadoop fs -moveToLocal [-crc] <src> <dst>
  ResultStatus moveToLocal(const std::string &src, const std::string &dest,
                           const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -mv URI [URI ...] <dest>
  ResultStatus mv(const std::string &src, const std::string &dest);
  // hadoop fs -rm [-f] [-r |-R] [-skipTrash] [-safely] URI [URI ...]
  ResultStatus rm(const std::string &paths, const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -rmdir [--ignore-fail-on-non-empty] URI [URI ...]
  ResultStatus rmdir(const std::string &dirs, const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -tail [-f] URI
  ResultStatus tail(std::string *output, const std::string &path,
                    const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -test -[defsz] URI
  ResultStatus test(const std::string &path, const folly::Optional<std::string> &optional = folly::none);
  // hadoop fs -text <src>
  ResultStatus text(std::string *output, const std::string &src);
  // hadoop fs -touchz URI [URI ...]
  ResultStatus touchz(const std::string &newfiles);
  //  hadoop fs -truncate [-w] <length> <paths>
  ResultStatus truncate(const std::string &paths, const std::string &length,
                        const folly::Optional<std::string> &optional = folly::none);

 private:
  ResultStatus invokePopen(std::string *result, const std::string &command);
};

}  // namespace hdfs
