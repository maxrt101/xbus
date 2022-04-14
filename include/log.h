#ifndef _XBUS_LOG_H_
#define _XBUS_LOG_H_ 1

#include <string>
#include <cstdarg>

namespace xbus {

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

LogLevel stringToLogLevel(const std::string& s);

void vflogf(FILE* dest, LogLevel level, bool printColon, const std::string& format, va_list args);
void vlogf(LogLevel level, bool printColon, const std::string& format, va_list args);
void logf(LogLevel level, bool printColon, const std::string format, ...);

void rdebug(const std::string format, ...);
void debug(const std::string format, ...);
void rinfo(const std::string format, ...);
void info(const std::string format, ...);
void rwarning(const std::string format, ...);
void warning(const std::string format, ...);
void rerror(const std::string format, ...);
void error(const std::string format, ...);
void rfatal(const std::string format, ...);
void fatal(const std::string format, ...);

LogLevel getLogLevel();
void setLogLevel(LogLevel level);

} /* namespace xbus */

#endif /* _XBUS_LOG_H_ */