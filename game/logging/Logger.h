
#pragma once

#include <functional>
#include <mutex>
#include <string_view>

enum class LogLevel : int { Error, Warn, Info, Debug };

class Logger {
 public:
  static void Error(std::wstring_view message) {
    Log(LogLevel::Error, message);
  }
  static void Warn(std::wstring_view message) {
    Log(LogLevel::Warn, message);
  }
  static void Info(std::wstring_view message) {
    Log(LogLevel::Info, message);
  }
  static void Debug(std::wstring_view message) {
    Log(LogLevel::Debug, message);
  }
  static void Log(LogLevel level, std::wstring_view message);

  static void SetSink(std::function<void(LogLevel, std::wstring_view)>);
  static void ClearSink();

 private:
  static std::mutex _mutex;
  static std::function<void(LogLevel, std::wstring_view)> _sink;
};
