#pragma once
#include <format>
#include <iostream>
#include <string_view>

namespace eng::core {

enum class LogLevel { Info, Warn, Error };

inline void logImpl(LogLevel level, std::string_view msg) {
    const char* color = (level == LogLevel::Info)  ? "\033[36m"
                      : (level == LogLevel::Warn)  ? "\033[33m"
                                                   : "\033[31m";
    const char* tag   = (level == LogLevel::Info)  ? "[INFO]"
                      : (level == LogLevel::Warn)  ? "[WARN]"
                                                   : "[ERR ]";
    std::cerr << color << tag << " \033[0m" << msg << '\n';
}

} // namespace eng::core

// Usage: LOG_INFO("Value: {}", myVar)
#define LOG_INFO(fmt, ...)  ::eng::core::logImpl(::eng::core::LogLevel::Info,  std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#define LOG_WARN(fmt, ...)  ::eng::core::logImpl(::eng::core::LogLevel::Warn,  std::format(fmt __VA_OPT__(,) __VA_ARGS__))
#define LOG_ERROR(fmt, ...) ::eng::core::logImpl(::eng::core::LogLevel::Error, std::format(fmt __VA_OPT__(,) __VA_ARGS__))
