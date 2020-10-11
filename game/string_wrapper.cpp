
#include "utils.h"

#include <string>

#include "string_wrapper.h"

NATIVE_API void WideStringResult_Delete(std::wstring *str) { delete str; }

NATIVE_API void StringResult_Delete(std::string *str) { delete str; }
