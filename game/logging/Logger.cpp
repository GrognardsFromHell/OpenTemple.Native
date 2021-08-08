
#include "Logger.h"

std::mutex Logger::_mutex;
std::function<void(LogLevel, std::wstring_view)> Logger::_sink;

void Logger::Log(LogLevel level, std::wstring_view message) {
  std::lock_guard<std::mutex> guard(_mutex);
  if (_sink) {
    _sink(level, message);
  }
}

void Logger::SetSink(std::function<void(LogLevel, std::wstring_view)> sink) {
  std::lock_guard<std::mutex> guard(_mutex);
  _sink = std::move(sink);
}

void Logger::ClearSink() {
  std::lock_guard<std::mutex> guard(_mutex);
  _sink = nullptr;
}
