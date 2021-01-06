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

#include "common/metrics/metrics.h"

#include "wdt_replicator.h"

namespace laser {

constexpr static char WDT_REPLICATOR_MONITOR_MODULE_NAME[] = "wdt_replicator";
constexpr static char WDT_REPLICATOR_MONITOR_WORK_TASK_SIZE_METRIC_NAME[] = "work_thread_pool_task_size";

void WdtReplicator::createTimeout(std::shared_ptr<WdtReplicatorManager> manager) {
  std::shared_ptr<WdtReplicator> replicator = shared_from_this();
  folly::EventBase* evb = manager->getEventBase();
  timeout_ = folly::AsyncTimeout::make(*evb, [replicator]() noexcept { replicator->abort_trigger_.store(true); });
  evb->runInEventBaseThread([replicator]() {
    replicator->timeout_->scheduleTimeout(std::chrono::milliseconds(replicator->timeval_));
  });
}

void WdtReplicator::cancelTimeout(std::shared_ptr<WdtReplicatorManager> manager) {
  std::shared_ptr<WdtReplicator> replicator = shared_from_this();
  manager->getEventBase()->runInEventBaseThread([replicator]() { replicator->timeout_->cancelTimeout(); });
}

bool WdtReplicator::receiver(std::string* connect_url, const std::string& host_name, const std::string& directory,
                             WdtNotifyCallback callback) {
  auto manager = weak_manager_.lock();
  if (!manager) {
    callback(name_space_, identifier_, facebook::wdt::ErrorCode::ERROR);
    return false;
  }
  auto& options = manager->getWdt().getWdtOptions();
  std::unique_ptr<facebook::wdt::WdtTransferRequest> reqPtr =
      std::make_unique<facebook::wdt::WdtTransferRequest>(options.start_port, options.num_ports, directory);
  reqPtr->hostName = host_name;

  auto abort = std::make_shared<facebook::wdt::WdtAbortChecker>(abort_trigger_);
  facebook::wdt::ErrorCode error = manager->getWdt().wdtReceiveStart(name_space_, *reqPtr, identifier_, abort);
  if (error != facebook::wdt::ErrorCode::OK) {
    LOG(ERROR) << "Transfer request error " << facebook::wdt::errorCodeToStr(error);
    callback(name_space_, identifier_, error);
    return false;
  }

  *connect_url = reqPtr->genWdtUrlWithSecret();
  LOG(INFO) << "Parsed url as " << reqPtr->getLogSafeString();
  try {
    std::shared_ptr<WdtReplicator> replicator = shared_from_this();
    manager->addUpdateTask([
      replicator,
      manager,
      callback = std::move(callback)
    ]() mutable {
       replicator->createTimeout(manager);
       SCOPE_EXIT {
         replicator->cancelTimeout(manager);
       };

       facebook::wdt::ErrorCode error =
           manager->getWdt().wdtReceiveFinish(replicator->name_space_, replicator->identifier_);
       if (error != facebook::wdt::ErrorCode::OK) {
         LOG(ERROR) << "Transfer request error " << facebook::wdt::errorCodeToStr(error);
       }
       callback(replicator->name_space_, replicator->identifier_, error);
     });
  }
  catch (...) {
    LOG(ERROR) << "Add wdt pool task fail.";
    callback(name_space_, identifier_, facebook::wdt::ErrorCode::ERROR);
    return false;
  }
  return true;
}

void WdtReplicator::sender(const std::string& connect_url, const std::string& directory, WdtNotifyCallback callback) {
  auto manager = weak_manager_.lock();
  if (!manager) {
    callback(name_space_, identifier_, facebook::wdt::ErrorCode::ERROR);
    return;
  }
  std::unique_ptr<facebook::wdt::WdtTransferRequest> reqPtr =
      std::make_unique<facebook::wdt::WdtTransferRequest>(connect_url);
  reqPtr->wdtNamespace = name_space_;
  reqPtr->destIdentifier = identifier_;
  reqPtr->directory = directory;
  LOG(INFO) << "Parsed url as " << reqPtr->getLogSafeString();

  try {
    std::shared_ptr<WdtReplicator> replicator = shared_from_this();
    manager->addUpdateTask([
      replicator,
      manager,
      req = std::move(reqPtr),
      callback = std::move(callback)
    ]() mutable {
       replicator->createTimeout(manager);
       SCOPE_EXIT {
         replicator->cancelTimeout(manager);
       };
       auto abort = std::make_shared<facebook::wdt::WdtAbortChecker>(replicator->abort_trigger_);
       facebook::wdt::ErrorCode error = manager->getWdt().wdtSend(*req, abort);
       if (error != facebook::wdt::ErrorCode::OK) {
         LOG(ERROR) << "Transfer request error " << facebook::wdt::errorCodeToStr(error);
       }
       callback(replicator->name_space_, replicator->identifier_, error);
     });
  }
  catch (...) {
    LOG(ERROR) << "Add wdt pool task fail.";
    callback(name_space_, identifier_, facebook::wdt::ErrorCode::ERROR);
  }
}

WdtReplicatorManager::WdtReplicatorManager(const std::string& app_name, uint32_t thread_nums) : app_name_(app_name) {
  event_thread_ = std::make_unique<WdtAbortTimerThread>();
  wdt_thread_pool_ = std::make_shared<folly::CPUThreadPoolExecutor>(
      thread_nums, std::make_shared<folly::NamedThreadFactory>("WdtSyncPool"));
  std::weak_ptr<folly::CPUThreadPoolExecutor> weak_pool = wdt_thread_pool_;
  metrics::Metrics::getInstance()->buildGauges(WDT_REPLICATOR_MONITOR_MODULE_NAME,
                                               WDT_REPLICATOR_MONITOR_WORK_TASK_SIZE_METRIC_NAME, 1000, [weak_pool]() {
    auto pool = weak_pool.lock();
    if (!pool) {
      return 0.0;
    }
    return static_cast<double>(pool->getTaskQueueSize());
  });
  facebook::wdt::Wdt::initializeWdt(app_name_);
}

void WdtReplicatorManager::stop() {
  if (wdt_thread_pool_) {
    wdt_thread_pool_->stop();
  }
}

WdtReplicatorManager::~WdtReplicatorManager() {
  stop();
  facebook::wdt::Wdt::releaseWdt(app_name_);
}

void WdtReplicatorManager::addUpdateTask(folly::Func func) { wdt_thread_pool_->add(std::move(func)); }

}  // namespace laser
