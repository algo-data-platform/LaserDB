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

#include "common/metrics/system_metrics.h"
#include <iostream>

#include "boost/filesystem.hpp"
#include "folly/FileUtil.h"
#include "folly/String.h"
#include "common/metrics/metrics.h"

namespace metrics {

constexpr static char PROC_SELF_FD_PATH[] = "/proc/self/fd";
constexpr static char PROC_SELF_STAT[] = "/proc/self/stat";
constexpr static char SYSTEM_METRIC_MODULE_NAME[] = "system";
constexpr static char SYSTEM_METRIC_FDS[] = "fds";
constexpr static char SYSTEM_METRIC_ARENA[] = "arena";
constexpr static char SYSTEM_METRIC_ORDBLKS[] = "ordblks";
constexpr static char SYSTEM_METRIC_SMBLKS[] = "smblks";
constexpr static char SYSTEM_METRIC_HBLKS[] = "hblks";
constexpr static char SYSTEM_METRIC_HBLKHD[] = "hblkhd";
constexpr static char SYSTEM_METRIC_USMBLKS[] = "usmblks";
constexpr static char SYSTEM_METRIC_FSMBLKS[] = "fsmblks";
constexpr static char SYSTEM_METRIC_UORDBLKS[] = "uordblks";
constexpr static char SYSTEM_METRIC_FORDBLKS[] = "fordblks";
constexpr static char SYSTEM_METRIC_KEEPCOST[] = "keepcost";
constexpr static char SYSTEM_METRIC_STATE[] = "state";
constexpr static char SYSTEM_METRIC_RSS[] = "rss";
constexpr static char SYSTEM_METRIC_VSIZE[] = "vsize";
constexpr static char SYSTEM_METRIC_THREAD_NUM[] = "thread_num";

folly::Singleton<SystemMetrics> global_service_status_system_metrics =
    folly::Singleton<SystemMetrics>().shouldEagerInit();
std::shared_ptr<SystemMetrics> SystemMetrics::getInstance() { return global_service_status_system_metrics.try_get(); }

void SystemMetrics::init() {
  std::vector<std::string> all_metrics({SYSTEM_METRIC_ARENA,    SYSTEM_METRIC_ORDBLKS,   SYSTEM_METRIC_SMBLKS,
                                        SYSTEM_METRIC_HBLKS,    SYSTEM_METRIC_HBLKHD,    SYSTEM_METRIC_USMBLKS,
                                        SYSTEM_METRIC_FSMBLKS,  SYSTEM_METRIC_UORDBLKS,  SYSTEM_METRIC_FORDBLKS,
                                        SYSTEM_METRIC_KEEPCOST, SYSTEM_METRIC_STATE,     SYSTEM_METRIC_RSS,
                                        SYSTEM_METRIC_VSIZE,    SYSTEM_METRIC_THREAD_NUM});

  // 采用 fds 这个 metrics 进行所有指标计算更新, 其他的指标直接获取
  auto metrics = metrics::Metrics::getInstance();
  std::weak_ptr<SystemMetrics> weak_system_metrics = shared_from_this();
  metrics->buildGauges(SYSTEM_METRIC_MODULE_NAME, SYSTEM_METRIC_FDS, 1000, [weak_system_metrics]() {
    auto system_metrics = weak_system_metrics.lock();
    if (!system_metrics) {
      return 0.0;
    }
    system_metrics->data_.withWLock([system_metrics](auto& data) { data = system_metrics->procStats(); });

    return system_metrics->data_.withRLock([](auto& data) {
      if (data.find(SYSTEM_METRIC_FDS) != data.end()) {
        return data.at(SYSTEM_METRIC_FDS);
      } else {
        return 0.0;
      }
    });
  });

  for (auto& t : all_metrics) {
    metrics->buildGauges(SYSTEM_METRIC_MODULE_NAME, t, 1000, [weak_system_metrics, t]() {
      auto system_metrics = weak_system_metrics.lock();
      if (!system_metrics) {
        return 0.0;
      }
      return system_metrics->data_.withRLock([t](auto& data) {
        if (data.find(t) != data.end()) {
          return data.at(t);
        } else {
          return 0.0;
        }
      });
    });
  }
}

size_t SystemMetrics::getProcessFileDescriptorNumber() {
  boost::filesystem::path proc_path(PROC_SELF_FD_PATH);
  size_t file_nums = 0;
  if (!boost::filesystem::exists(proc_path)) {
    return file_nums;
  }

  boost::filesystem::directory_iterator end_iter;
  for (boost::filesystem::directory_iterator iter(proc_path); iter != end_iter; iter++) {
    try {
      if (!boost::filesystem::is_directory(*iter)) {
        file_nums++;
      }
    }
    catch (...) {
    }
  }
  return file_nums;
}

const std::unordered_map<std::string, uint32_t> SystemMetrics::mallInfo() {
  struct mallinfo mi = mallinfo();
  std::unordered_map<std::string, uint32_t> info;
  info[SYSTEM_METRIC_ARENA] = mi.arena;        // The total amount of memory allocated by means other than mmap(2)
  info[SYSTEM_METRIC_ORDBLKS] = mi.ordblks;    // The number of ordinary (i.e., non-fastbin) free blocks.
  info[SYSTEM_METRIC_SMBLKS] = mi.smblks;      // The number of fastbin free blocks
  info[SYSTEM_METRIC_HBLKS] = mi.hblks;        // The number of blocks currently allocated using mmap(2).
  info[SYSTEM_METRIC_HBLKHD] = mi.hblkhd;      // The number of bytes in blocks currently allocated using mmap(2)
  info[SYSTEM_METRIC_USMBLKS] = mi.usmblks;    // unused
  info[SYSTEM_METRIC_FSMBLKS] = mi.fsmblks;    // The total number of bytes in fastbin free blocks.
  info[SYSTEM_METRIC_UORDBLKS] = mi.uordblks;  // The total number of bytes used by in-use allocations.
  info[SYSTEM_METRIC_FORDBLKS] = mi.fordblks;  // The total number of bytes in free blocks.
  info[SYSTEM_METRIC_KEEPCOST] = mi.keepcost;  // The total amount of releasable free space at the top of the heap.
  return info;
}

const std::unordered_map<std::string, double> SystemMetrics::procStats() {
  std::unordered_map<std::string, double> result;
  std::string stat_str;

  if (!folly::readFile(PROC_SELF_STAT, stat_str)) {
    return result;
  }

  std::vector<std::string> stats;
  folly::split(" ", stat_str, stats);
  if (stats.size() < 24) {
    return result;
  }

  std::unordered_map<std::string, double> stateMap({{"R", 1}, {"S", 2}, {"D", 3}, {"T", 4}, {"Z", 5}, {"X", 6}});
  if (stateMap.find(stats.at(2)) == stateMap.end()) {
    result[SYSTEM_METRIC_STATE] = 7.0;
  } else {
    result[SYSTEM_METRIC_STATE] = stateMap.at(stats.at(2));
  }

  // thread num 19
  auto threadNumConv = folly::tryTo<double>(stats.at(19));
  if (!threadNumConv.hasValue()) {
    return result;
  }
  result[SYSTEM_METRIC_THREAD_NUM] = threadNumConv.value();

  uint32_t pageSize = static_cast<double>(sysconf(_SC_PAGESIZE));
  // rss 23
  auto rssConv = folly::tryTo<double>(stats.at(23));
  if (!rssConv.hasValue()) {
    return result;
  }
  result[SYSTEM_METRIC_RSS] = rssConv.value() * pageSize;
  // vsize 22
  auto vsizeConv = folly::tryTo<double>(stats.at(22));
  if (!vsizeConv.hasValue()) {
    return result;
  }
  result[SYSTEM_METRIC_VSIZE] = vsizeConv.value();
  // fd num
  result[SYSTEM_METRIC_FDS] = static_cast<double>(getProcessFileDescriptorNumber());

  std::unordered_map<std::string, uint32_t> mallinfo = mallInfo();
  for (auto& t : mallinfo) {
    result[t.first] = static_cast<double>(t.second);
  }

  return result;
}

}  // namespace metrics
