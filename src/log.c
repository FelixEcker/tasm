#include <log.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void log_dbg(const char *format, ...) {
  printf("%s ", LOG_PREFIX_DBG);
  va_list arg;

  va_start(arg, format);
  vfprintf(stdout, format, arg);
  va_end(arg);
  printf(ANSI_RESET);
}

void log_inf(const char *format, ...) {
  printf("%s ", LOG_PREFIX_INF);
  va_list arg;

  va_start(arg, format);
  vfprintf(stdout, format, arg);
  va_end(arg);
  printf(ANSI_RESET);
}

void log_wrn(const char *format, ...) {
  printf("%s ", LOG_PREFIX_WRN);
  va_list arg;

  va_start(arg, format);
  vfprintf(stdout, format, arg);
  va_end(arg);
  printf(ANSI_RESET);
}

void log_err(const char *format, ...) {
  printf("%s ", LOG_PREFIX_ERR);
  va_list arg;

  va_start(arg, format);
  vfprintf(stdout, format, arg);
  va_end(arg);
  printf(ANSI_RESET);
}

