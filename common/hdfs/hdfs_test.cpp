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

#include "folly/init/Init.h"
#include "folly/portability/GFlags.h"
#include "gflags/gflags.h"
#include "hdfs.h"

DEFINE_string(directory_name, "/user/test", "hdfs client command option");
DEFINE_string(file_name, "/user/test/slaves", "hdfs file name");
DEFINE_string(localpath, "./", "hdfs command local file path");
DEFINE_string(chown_owner_group, "jianjian5:jianjian5", "hdfs chown command");

// test class for hdfs client api
namespace hdfs {

class HdfsCmdTest {
 public:
  HdfsCmdTest() {}
  ~HdfsCmdTest() {}
  // hadoop fs -get [option] <src> <dest>
  void test_get() {
    // const std::string option = FLAGS_option3;
    ResultStatus ret = cmd_.get(FLAGS_file_name, FLAGS_localpath);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop get command succ";
    } else {
      LOG(ERROR) << "hadoop get command fail";
    }
  }
  // hadoop fs -checksum <src>
  void test_checksum() {
    std::vector<std::string> output;
    ResultStatus ret = cmd_.checksum(&output, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop checksum succ";
      LOG(INFO) << "checksum value as follow:";
      for (auto ck : output) {
        LOG(INFO) << ck;
      }
    } else {
      LOG(ERROR) << "hadoop checksum command fail";
    }
  }
  // hadoop put
  void test_put() {
    ResultStatus ret = cmd_.put(FLAGS_localpath, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop put succ";
    } else {
      LOG(ERROR) << "hadoop put comamnd fail";
    }
  }

  // add hadoop cat
  void test_cat() {
    std::string output;
    ResultStatus ret = cmd_.cat(&output, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop cat succ";
      LOG(INFO) << output;
    } else {
      LOG(ERROR) << "hadoop cat command fail";
    }
  }

  // hadoop count
  void test_count() {
    HdfsCount count;
    ResultStatus ret = cmd_.count(&count, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop count succ";
      LOG(INFO) << "hadoop output as follow:";
      LOG(INFO) << count.dirs << "\t" << count.files << "\t" << count.words;
    } else {
      LOG(ERROR) << "hadoop count command fail";
    }
  }

  // hadoop du
  void test_du() {
    std::vector<std::pair<uint64_t, std::string>> output;
    ResultStatus ret = cmd_.du(&output, FLAGS_directory_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop du succ";
      LOG(INFO) << "hadoop du output as follow:";
      for (int i = 0; i < output.size(); i++) {
        std::pair<uint64_t, std::string> cur = output[i];
        LOG(INFO) << cur.first << " \t" << cur.second;
      }
    } else {
      LOG(ERROR) << "hadoop du command fail";
    }
  }

  void test_df() {
    std::vector<HdfsDf> output;
    ResultStatus ret = cmd_.df(&output, FLAGS_directory_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop df succ";
      LOG(INFO) << "hadoop output as follow: ";
      for (auto each : output) {
        LOG(INFO) << each.filesystem << "\t" << each.total_capacity << "\t" << each.used_capacity << "\t"
                  << each.free_capacity;
      }
    } else {
      LOG(ERROR) << "hadoop df command fail";
    }
  }

  void test_ls() {
    std::vector<HdfsLs> output;
    ResultStatus ret = cmd_.ls(&output, FLAGS_directory_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop ls succ";
      LOG(INFO) << "hadoop ls output as follow:";
      for (auto cur : output) {
        LOG(INFO) << cur.file_words << "\t" << cur.file_generated_time << "\t" << cur.file_name;
      }
    } else {
      LOG(ERROR) << "hadoop ls command fail";
    }
  }

  void test_tail() {
    std::string output;
    ResultStatus ret = cmd_.tail(&output, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop tail succ";
      LOG(INFO) << "hadoop tail output is :" << output;
    } else {
      LOG(ERROR) << "hadoop tail command fail";
    }
  }

  void test_text() {
    std::string output;
    ResultStatus ret = cmd_.text(&output, FLAGS_file_name);
    if (ret == ResultStatus::RUN_SUCC) {
      LOG(INFO) << "hadoop text succ";
      LOG(INFO) << "hadoop text output is :" << output;
    } else {
      LOG(ERROR) << "hadoop text command fail";
    }
  }

 private:
  typedef hdfs::HdfsCmd hdfs_cmd;
  hdfs_cmd cmd_;
};
}  // namespace hdfs

int main(int argc, char *argv[]) {
  // hadoop command test
  folly::init(&argc, &argv);
  FLAGS_logtostderr = true;
  hdfs::HdfsCmdTest m_hdfs;
  m_hdfs.test_get();
  m_hdfs.test_checksum();
  //  m_hdfs.test_put();
  m_hdfs.test_cat();
  m_hdfs.test_count();
  m_hdfs.test_du();
  m_hdfs.test_df();
  m_hdfs.test_ls();
  m_hdfs.test_tail();
  m_hdfs.test_text();
  return 0;
}
