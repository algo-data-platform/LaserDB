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

#include "laser/if/gen-cpp2/laser_types.h"

namespace laser {

struct StatusMetaInfo {
  Status status;
  std::string status_name;
  std::string message;
};

class StatusInfo {
 public:
  static StatusMetaInfo meta_info_list[];
};

const std::string statusToMessage(const Status& status);
const std::string statusToName(const Status& status);
std::ostream& operator<<(std::ostream& os, const Status& status);
void throwLaserException(const Status& status, const std::string& message = "");
const LaserException createLaserException(const Status& status, const std::string& message);

}  // namespace laser
