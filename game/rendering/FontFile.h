
#pragma once

#include <string>
#include <utility>
#include <vector>

struct FontFile {
  const std::string filename;
  const std::vector<uint8_t> data;

  FontFile(std::string filename, std::vector<uint8_t>  data)
      : filename(std::move(filename)), data(std::move(data)) {}
};
