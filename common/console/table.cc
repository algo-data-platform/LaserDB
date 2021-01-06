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

#include "common/console/table.h"

namespace common {
namespace console {

constexpr static char CONSOLE_TABLE_HEADER_SPLIT = '=';
constexpr static char CONSOLE_TABLE_ROW_SPLIT = '-';
constexpr static char CONSOLE_TABLE_COL_SPLIT = '|';
constexpr static char CONSOLE_TABLE_NO_DATA[] = "No data";

void ConsoleTable::setHeader(const std::vector<ConsoleTableCell> headers) {
  if (data_.empty()) {
    data_.push_back(headers);
  } else {
    data_[0] = headers;
  }
}

void ConsoleTable::addRows(const std::vector<ConsoleTableCell> cols) { data_.push_back(cols); }

void ConsoleTable::print(std::ostream& os) {
  if (data_.empty()) {
    os << CONSOLE_TABLE_NO_DATA << std::endl;
  }

  int cols = data_[0].size();
  std::vector<int> max_widths(cols);
  for (int i = 0; i < cols; i++) {
    int max_width = 0;
    for (int j = 0; j < data_.size(); j++) {
      max_width = std::max(static_cast<int>(data_[j][i].getText().size()), max_width);
    }
    max_widths[i] = max_width;
  }

  int total_width = std::accumulate(max_widths.begin(), max_widths.end(), cols + 1);
  char prev = os.fill(CONSOLE_TABLE_HEADER_SPLIT);
  os.width(total_width);
  os << "" << std::endl;
  os.fill(prev);

  for (int i = 0; i < data_.size(); i++) {
    os.fill(prev);
    os.width(1);
    os << CONSOLE_TABLE_COL_SPLIT;
    for (int j = 0; j < cols; j++) {
      printCell(os, data_[i][j], max_widths[j]);
    }
    os << "" << std::endl;
    os.fill(CONSOLE_TABLE_ROW_SPLIT);
    os.width(total_width);
    os << "" << std::endl;
  }
  os.fill(prev);
}

void ConsoleTable::printCell(std::ostream& os, const ConsoleTableCell& cell, int width) {
  switch (cell.getColor()) {
    case ConsoleTableColor::GREY:
      os << termcolor::grey;
      break;
    case ConsoleTableColor::RED:
      os << termcolor::red;
      break;
    case ConsoleTableColor::GREEN:
      os << termcolor::green;
      break;
    case ConsoleTableColor::BLUE:
      os << termcolor::blue;
      break;
    case ConsoleTableColor::YELLOW:
      os << termcolor::yellow;
      break;
    case ConsoleTableColor::MAGENTA:
      os << termcolor::magenta;
      break;
    case ConsoleTableColor::CYAN:
      os << termcolor::cyan;
      break;
    case ConsoleTableColor::WHITE:
      os << termcolor::white;
      break;
    default:
      break;
  }
  os.width(width);
  os << cell.getText() << termcolor::reset << CONSOLE_TABLE_COL_SPLIT;
}

}  // namespace console
}  // namespace common
