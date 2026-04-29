//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <vector>
#include <map>

struct StreamEntry {
  std::string id;
  std::map<std::string, std::string> fields;
};

struct Stream {
  std::vector<StreamEntry> entries;
};
