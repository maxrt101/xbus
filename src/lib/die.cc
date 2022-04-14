#include <xbus/die.h>
#include <mrt/console/colors.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

[[noreturn]] void xbus::die(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "[%sERROR%s]: ", mrt::console::RED, mrt::console::RESET);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}