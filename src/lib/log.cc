#include <xbus/log.h>
#include <mrt/console/colors.h>

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <mutex>

#define _VLOGF_INTERNAL(printColon, level) \
  do { \
    va_list args; \
    va_start(args, format); \
    vlogf(level, printColon, format, args); \
    va_end(args); \
  } while (0)

static xbus::LogLevel g_logLevel = xbus::LogLevel::INFO;
static std::mutex g_ioMutex;

xbus::LogLevel xbus::stringToLogLevel(const std::string& s) {
  std::string str = s;
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  if (str == "debug") {
    return LogLevel::DEBUG;
  } else if (str == "info") {
    return LogLevel::INFO;
  } else if (str == "warning") {
    return LogLevel::WARNING;
  } else if (str == "error") {
    return LogLevel::ERROR;
  } else if (str == "fatal") {
    return LogLevel::FATAL;
  }
  return LogLevel::DEBUG;
}

void xbus::vflogf(FILE* dest, LogLevel level, bool printColon, const std::string& format, va_list args) {
  if (level < g_logLevel) return;
  std::unique_lock ioLock(g_ioMutex);

  ::fprintf(dest, "[");
  switch (level) {
    case LogLevel::FATAL:   ::fprintf(dest, "%sFATAL",   mrt::console::RED_RED); break;
    case LogLevel::ERROR:   ::fprintf(dest, "%sERROR",   mrt::console::RED);     break;
    case LogLevel::WARNING: ::fprintf(dest, "%sWARNING", mrt::console::YELLOW);  break;
    case LogLevel::INFO:    ::fprintf(dest, "%sINFO",    mrt::console::CYAN);    break;
    case LogLevel::DEBUG:   ::fprintf(dest, "%sDEBUG",   mrt::console::BLUE);    break;
    default:                ::fprintf(dest, "<>");                               break;
  }

  ::fprintf(dest, "%s]", mrt::console::RESET);
  if (printColon) {
    ::fprintf(dest, ": ");
  }
  ::vfprintf(dest, format.c_str(), args);
  ::fprintf(dest, "\n");
}

void xbus::vlogf(LogLevel level, bool printColon, const std::string& format, va_list args) {
  FILE* dest = stdout;
  if (level > LogLevel::INFO) {
    dest = stderr;
  }

  vflogf(dest, level, printColon, format, args);
}

void xbus::logf(LogLevel level, bool printColon, const std::string format, ...) {
  va_list args;
  va_start(args, format);
  vlogf(level, printColon, format, args);
  va_end(args);
}

void xbus::debug(const std::string format, ...) {
  _VLOGF_INTERNAL(true, LogLevel::DEBUG);
}

void xbus::info(const std::string format, ...) {
  _VLOGF_INTERNAL(true, LogLevel::INFO);
}

void xbus::warning(const std::string format, ...) {
  _VLOGF_INTERNAL(true, LogLevel::WARNING);
}

void xbus::error(const std::string format, ...) {
  _VLOGF_INTERNAL(true, LogLevel::ERROR);
}

void xbus::fatal(const std::string format, ...) {
  _VLOGF_INTERNAL(true, LogLevel::FATAL);
}

void xbus::rdebug(const std::string format, ...) {
  _VLOGF_INTERNAL(false, LogLevel::DEBUG);
}

void xbus::rinfo(const std::string format, ...) {
  _VLOGF_INTERNAL(false, LogLevel::INFO);
}

void xbus::rwarning(const std::string format, ...) {
  _VLOGF_INTERNAL(false, LogLevel::WARNING);
}

void xbus::rerror(const std::string format, ...) {
  _VLOGF_INTERNAL(false, LogLevel::ERROR);
}

void xbus::rfatal(const std::string format, ...) {
  _VLOGF_INTERNAL(false, LogLevel::FATAL);
}

xbus::LogLevel xbus::getLogLevel() {
  return g_logLevel;
}

void xbus::setLogLevel(LogLevel level) {
  g_logLevel = level;
}
