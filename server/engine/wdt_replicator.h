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
 * @author Deyun Yang <yangdeyunx@gmail.com>
 */

#pragma once

#include "folly/SpinLock.h"
#include "folly/ScopeGuard.h"
#include "folly/io/async/AsyncTimeout.h"
#include "folly/io/async/ScopedEventBaseThread.h"
#include "folly/executors/CPUThreadPoolExecutor.h"
#include "folly/Function.h"
#include "Wdt.h"

namespace laser {

using WdtNotifyCallback = folly::Function<void(std::string& /* name_space */, std::string& /* identifier */,
                                               facebook::wdt::ErrorCode)>;

class WdtReplicatorManager;
class WdtReplicator : public std::enable_shared_from_this<WdtReplicator> {
 public:
  WdtReplicator(std::weak_ptr<WdtReplicatorManager> weak_manager, const std::string& name_space,
                const std::string& identifier, uint32_t timeout)
      : weak_manager_(weak_manager), name_space_(name_space), identifier_(identifier), timeval_(timeout) {}
  virtual ~WdtReplicator() = default;
  virtual bool receiver(std::string* connect_url, const std::string& host_name, const std::string& directory,
                        WdtNotifyCallback callback);
  virtual void sender(const std::string& connect_url, const std::string& directory, WdtNotifyCallback callback);
  virtual void abort() { abort_trigger_.store(true); }

 private:
  std::weak_ptr<WdtReplicatorManager> weak_manager_;
  std::string name_space_;
  std::string identifier_;
  std::atomic<bool> abort_trigger_{false};
  std::unique_ptr<folly::AsyncTimeout> timeout_;
  uint32_t timeval_{1000};

  void createTimeout(std::shared_ptr<WdtReplicatorManager> manager);
  void cancelTimeout(std::shared_ptr<WdtReplicatorManager> manager);
};

class WdtAbortTimerThread : public folly::ScopedEventBaseThread {
 public:
  WdtAbortTimerThread() : folly::ScopedEventBaseThread("WdtAbortTimerThread") {}
  ~WdtAbortTimerThread() { VLOG(5) << "Monitor timer thread delete."; }
};

class WdtReplicatorManager {
 public:
  WdtReplicatorManager(const std::string& app_name, uint32_t threads_num);
  virtual ~WdtReplicatorManager();
  virtual folly::EventBase* getEventBase() { return event_thread_->getEventBase(); }
  virtual void addUpdateTask(folly::Func func);
  virtual inline facebook::wdt::Wdt& getWdt() { return facebook::wdt::Wdt::getWdt(app_name_); }
  virtual void stop();

 private:
  std::unique_ptr<WdtAbortTimerThread> event_thread_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> wdt_thread_pool_;
  std::string app_name_;
};

}  // namespace laser
