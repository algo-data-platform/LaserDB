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

#include <iomanip>
#include <ctime>
#include <chrono>  // NOLINT
#include <string>

#include "folly/dynamic.h"
#include "folly/json.h"
#include "folly/GLog.h"

namespace common {

bool fromJson(folly::dynamic* result, const folly::StringPiece& json);

bool toJson(std::string* result, const folly::dynamic& data);

std::string pathJoin(const std::string &a, const std::string &b);

std::time_t currentTimeInMs();

std::time_t currentTimeInNs();

std::string currentTimeToStr(std::time_t cur_time);

// get current time and format it as requested
std::string currentTimeInFormat(const std::string &format);

uint32_t ipToInt(const std::string& ip);

void getLocalIpAddress(std::string* ip_addr);

}  // namespace common
