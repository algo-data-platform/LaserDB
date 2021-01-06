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

#pragma once

#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>

#include "common/console/termcolor.h"

namespace common {
namespace console {

enum class ConsoleTableColor {
  GREY,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  WHITE,
  DEFAULT
};

class ConsoleTableCell {
 public:
  explicit ConsoleTableCell(const std::string& text, ConsoleTableColor color = ConsoleTableColor::DEFAULT)
      : text_(text), color_(color) {}

  const ConsoleTableColor& getColor() const { return color_; }
  const std::string& getText() const { return text_; }

 private:
  std::string text_;
  ConsoleTableColor color_;
};

//
// 在 Console 下输出有格式带色彩的简单的表格
// 例如下面的表格：
//
// =====================
// |Heartbeat|Candidate|
// ---------------------
// |       NO|       NO|
// ---------------------
// |      YES|      YES|
// ---------------------
//
// ConsoleTable table;
// table.setHeader({
//  ConsoleTableCell("Heartbeat"),
//  ConsoleTableCell("Candidate")
// });
//
// table.addRows({
//  ConsoleTableCell("NO"),
//  ConsoleTableCell("NO"),
// });
// table.addRows({
//  ConsoleTableCell("YES"),
//  ConsoleTableCell("YES"),
// });
//
// table.print(std::cout);
//
class ConsoleTable {
 public:
  ConsoleTable() = default;
  ~ConsoleTable() = default;
  ConsoleTable(const ConsoleTable&) = default;
  ConsoleTable(ConsoleTable&&) = default;
  ConsoleTable& operator=(const ConsoleTable&) = default;

  void setHeader(const std::vector<ConsoleTableCell> headers);
  void addRows(const std::vector<ConsoleTableCell> cols);
  void print(std::ostream& os);

 private:
  std::vector<std::vector<ConsoleTableCell>> data_;
  void printCell(std::ostream& os, const ConsoleTableCell& cell, int width);
};

}  // namespace console
}  // namespace common
