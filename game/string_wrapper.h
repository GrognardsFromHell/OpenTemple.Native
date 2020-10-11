
#pragma once

#include <cassert>
#include <string>
#include <limits>

// This struct is used on the C# side as well,
// and allows access to "data" and "length" without
// further P/Invoke calls.
template <typename T>
struct Result {
  const void *data;
  int length;
  T *string;

  Result<T> &operator=(T *str) {
    if (str) {
      assert(str->size() < std::numeric_limits<int>::max());
      data = str->data();
      length = (int)str->size();
      string = str;
    } else {
      data = nullptr;
      length = 0;
      string = nullptr;
    }
    return *this;
  }

  ~Result() { delete string; }
};

using StringResult = Result<std::string>;
using WideStringResult = Result<std::wstring>;
