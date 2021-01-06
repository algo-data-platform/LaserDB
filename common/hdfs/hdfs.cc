/*
 * Copyright 2020 Weibo Inc.
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

#include "hdfs.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "folly/Subprocess.h"

namespace hdfs {

DEFINE_string(hadoop_path, "hadoop", "hadoop bin file path");

constexpr char HDFS_CMD_CHECKSUM[] = " fs -checksum ";
constexpr char HDFS_CMD_GET[] = " fs -get ";
constexpr char HDFS_CMD_PUT[] = " fs -put ";
constexpr char HDFS_CMD_CP[] = " fs -cp ";
constexpr char HDFS_CMD_MV[] = " fs -mv ";
constexpr char HDFS_CMD_APPENDTOFILE[] = " fs -appendToFile ";
constexpr char HDFS_CMD_CAT[] = " fs -cat ";
constexpr char HDFS_CMD_CHGRP[] = " fs -chgrp ";
constexpr char HDFS_CMD_CHMOD[] = " fs -chmod ";
constexpr char HDFS_CMD_CHOWN[] = " fs -chown ";
constexpr char HDFS_CMD_COPYFROMLOCAL[] = " fs -copyFromLocal ";
constexpr char HDFS_CMD_COPYTOLOCAL[] = " fs -copyToLocal ";
constexpr char HDFS_CMD_COUNT[] = " fs -count ";
constexpr char HDFS_CMD_DF[] = " fs -df ";
constexpr char HDFS_CMD_DU[] = " fs -du ";
constexpr char HDFS_CMD_GETMERGE[] = " fs -getmerge ";
constexpr char HDFS_CMD_LS[] = " fs -ls ";
constexpr char HDFS_CMD_MKDIR[] = " fs -mkdir ";
constexpr char HDFS_CMD_MOVETOLOCAL[] = " fs -moveToLocal";
constexpr char HDFS_CMD_MOVEFROMLOCAL[] = " fs -moveFromLocal ";
constexpr char HDFS_CMD_RM[] = " fs -rm ";
constexpr char HDFS_CMD_RMDIR[] = " fs -rmdir ";
constexpr char HDFS_CMD_TAIL[] = " fs -tail ";
constexpr char HDFS_CMD_TEST[] = " fs -test ";
constexpr char HDFS_CMD_TEXT[] = " fs -text ";
constexpr char HDFS_CMD_TOUCHZ[] = " fs -touchz ";
constexpr char HDFS_CMD_TRUNCATE[] = " fs -truncate ";

ResultStatus HdfsCmd::invokePopen(std::string *result, const std::string &command) {
  if (command.empty()) {
    return ResultStatus::NONE;
  }

  auto options = folly::Subprocess::Options().pipeStdout().pipeStderr();
  VLOG(5) << "start popen command :" << command;
  folly::Subprocess proc(command, options);
  auto pipe = proc.communicate();

  result->clear();
  result->append(pipe.first);
  auto status = proc.wait();
  if (!status.exited()) {
    LOG(ERROR) << "hadoop shell exec fail, err:" << status.str();
    return ResultStatus::RUN_FAIL;
  }

  if (status.exitStatus() == 0) {
    VLOG(5) << "hadoop shell output is:" << *result;
    return ResultStatus::RUN_SUCC;
  }

  LOG(INFO) << "Run cmd:" << command << " status:" << status.exitStatus() << " err:" << pipe.second;
  return ResultStatus::RUN_FAIL;
}

ResultStatus HdfsCmd::checksum(std::vector<std::string> *checksum, const std::string &path) {
  std::string cmd{FLAGS_hadoop_path}, command_output;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_CHECKSUM, path);
  ResultStatus status = invokePopen(&command_output, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }
  // 命令产生的输出形式；filename，md5---  xxxxx 等
  // 此处解析去掉filename，保存后面的值作为checksum
  folly::split('\t', command_output, *checksum, true);
  if (checksum->size() != 3) {
    LOG(ERROR) << "checksum value not correct";
    return ResultStatus::RUN_FAIL;
  } else {
    checksum->erase(checksum->begin());
  }
  return status;
}

ResultStatus HdfsCmd::get(const std::string &src, const std::string &dest, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_GET, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_GET, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::put(const std::string &src, const std::string &dest, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_PUT, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_PUT, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::cp(const std::string &src, const std::string &dest, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CP, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CP, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::mv(const std::string &src, const std::string &dest, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MV, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MV, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::appendToFile(const std::string &src, const std::string &dest) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_APPENDTOFILE, src, " ", dest);
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::cat(std::string *result, const std::string &path, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CAT, option.value(), " ", path);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CAT, path);
  }
  ResultStatus status = invokePopen(&ret, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }
  result->append(ret);
  return status;
}

ResultStatus HdfsCmd::chgrp(const std::string &paths, const std::string &group_name,
                            const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHGRP, option.value(), " ", group_name, " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHGRP, group_name, " ", paths);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::chmod(const std::string &paths, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHMOD, option.value(), " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHMOD, paths);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::chown(const std::string &paths, const std::string &group_user,
                            const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHOWN, option.value(), " ", group_user, " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_CHOWN, group_user, " ", paths);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::copyFromLocal(const std::string &src, const std::string &dest,
                                    const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COPYFROMLOCAL, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COPYFROMLOCAL, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::copyToLocal(const std::string &src, const std::string &dest,
                                  const folly::Optional<std::string> &optional) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (optional.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COPYTOLOCAL, optional.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COPYTOLOCAL, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::count(HdfsCount *output, const std::string &paths, const folly::Optional<std::string> &optional) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (optional.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COUNT, optional.value(), paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_COUNT, paths);
  }
  ResultStatus status = invokePopen(&ret, cmd);
  if (status == ResultStatus::RUN_SUCC) {
    return status;
  }
  // hadoop count 的输出为目录数，文件数，字符数，该函数作用为
  // 解析出这三个数，并保存到output结构体中
  std::vector<std::string> result;
  folly::split(' ', ret, result, true);
  if (static_cast<uint32_t>(result.size()) != 4) {
    return ResultStatus::RUN_FAIL;
  }
  output->dirs = folly::to<uint32_t>(result[0]);
  output->files = folly::to<uint32_t>(result[1]);
  output->words = folly::to<uint32_t>(result[2]);
  return status;
}

ResultStatus HdfsCmd::df(std::vector<HdfsDf> *output, const std::string &paths,
                         const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_DF, option.value(), paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_DF, paths);
  }
  ResultStatus status = invokePopen(&ret, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }

  std::vector<std::string> input;      // shell 的所有输出
  std::vector<std::string> each_line;  //输出切分后的每行内容
  HdfsDf tmp;
  folly::split('\n', ret, input, true);
  for (size_t i = 1; i < input.size(); i++) {  // 首行为字段名标记，略过
    folly::split(' ', input[i], each_line, true);
    if (static_cast<uint32_t>(each_line.size()) != 5) {
      return ResultStatus::RUN_FAIL;
    }
    tmp.filesystem = each_line[0];
    tmp.total_capacity = folly::to<uint64_t>(each_line[1]);
    tmp.used_capacity = folly::to<uint64_t>(each_line[2]);
    tmp.free_capacity = folly::to<uint64_t>(each_line[3]);
    output->push_back(tmp);
    each_line.clear();
  }
  return status;
}

ResultStatus HdfsCmd::du(std::vector<std::pair<uint64_t, std::string>> *output, const std::string &paths,
                         const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_DU, option.value(), paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_DU, paths);
  }
  ResultStatus status = invokePopen(&ret, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }
  std::vector<std::string> all_lines;
  std::vector<std::string> each_line;
  // 解析du的输出
  folly::split('\n', ret, all_lines, true);
  for (auto each : all_lines) {
    folly::split(' ', each, each_line, true);
    if (static_cast<uint32_t>(each_line.size()) != 2) {
      return ResultStatus::RUN_FAIL;
    }
    output->push_back(make_pair(folly::to<uint64_t>(each_line[0]), each_line[1]));
    each_line.clear();
  }
  return status;
}

ResultStatus HdfsCmd::getmerge(const std::string &src, const std::string &dest,
                               const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_GETMERGE, option.value(), " ", src, " ", dest);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_GETMERGE, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::ls(std::vector<HdfsLs> *output, const std::string &path,
                         const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_LS, option.value(), " ", path);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_LS, path);
  }
  ResultStatus status = invokePopen(&ret, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }
  // 解析ls 命令的输出file的 size/ time / name
  std::vector<std::string> all_lines;
  std::vector<std::string> each_line;
  HdfsLs hdfsls;
  folly::split('\n', ret, all_lines, true);
  // 显示目录时，首行可删除，内容忽略
  if (static_cast<uint32_t>(all_lines.size()) > 1) {
    all_lines.erase(all_lines.begin());
  }
  for (auto each : all_lines) {
    folly::split(' ', each, each_line, true);
    if (static_cast<uint32_t>(each_line.size()) != 8) {
      return ResultStatus::RUN_FAIL;
    }
    hdfsls.file_words = folly::to<uint64_t>(each_line[4]);
    hdfsls.file_generated_time = folly::to<std::string>(each_line[5], " ", each_line[6]);
    hdfsls.file_name = each_line[7];
    // first column: directory starts with 'd' while file starts with '-'
    hdfsls.type = each_line[0][0] == 'd' ? FileType::DIRECTORY : FileType::FILE;

    output->push_back(hdfsls);
    each_line.clear();
  }
  return status;
}

ResultStatus HdfsCmd::mkdir(const std::string &paths, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MKDIR, option.value(), " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MKDIR, paths);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::moveFromLocal(const std::string &src, const std::string &dest) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_MOVEFROMLOCAL, src, " ", dest);
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::moveToLocal(const std::string &src, const std::string &dest,
                                  const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MOVETOLOCAL, option.value(), " ", src);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_MOVETOLOCAL, src, " ", dest);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::mv(const std::string &src, const std::string &dest) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_MV, src, " ", dest);
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::rm(const std::string &paths, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_RM, option.value(), " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_RM, paths);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::rmdir(const std::string &dirs, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, ret;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_RMDIR, option.value(), " ", dirs);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_RMDIR, dirs);
  }
  return invokePopen(&ret, cmd);
}

ResultStatus HdfsCmd::tail(std::string *output, const std::string &path, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, result;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TAIL, option.value(), " ", path);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TAIL, path);
  }
  ResultStatus status = invokePopen(&result, cmd);
  output->append(result);
  VLOG(5) << "hadoop command tail output is :" << *output;
  return status;
}

ResultStatus HdfsCmd::test(const std::string &path, const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, result;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TEST, option.value(), " ", path);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TEST, path);
  }
  return invokePopen(&result, cmd);
}

ResultStatus HdfsCmd::text(std::string *output, const std::string &src) {
  std::string cmd{FLAGS_hadoop_path}, result;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_TEXT, src);
  ResultStatus status = invokePopen(&result, cmd);
  if (status != ResultStatus::RUN_SUCC) {
    return status;
  }
  output->append(result);
  VLOG(5) << "hadoop command text output is :" << *output;
  return status;
}

ResultStatus HdfsCmd::touchz(const std::string &newfiles) {
  std::string cmd{FLAGS_hadoop_path}, result;
  cmd = folly::to<std::string>(cmd, HDFS_CMD_TOUCHZ, newfiles);
  return invokePopen(&result, cmd);
}

ResultStatus HdfsCmd::truncate(const std::string &paths, const std::string &length,
                               const folly::Optional<std::string> &option) {
  std::string cmd{FLAGS_hadoop_path}, result;
  if (option.has_value()) {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TRUNCATE, option.value(), " ", length, " ", paths);
  } else {
    cmd = folly::to<std::string>(cmd, HDFS_CMD_TRUNCATE, " ", length, paths);
  }
  return invokePopen(&result, cmd);
}

}  // namespace hdfs
