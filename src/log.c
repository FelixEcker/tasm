#include <log.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void _log(const char *format, ...) {
  va_list arg;

  va_start(arg, format);
  vfprintf(stdout, format, arg);
  va_end(arg);
}

void log_dbg(const char *format, ...) {
  printf("%s ", LOG_PREFIX_DBG);
  _log(format);
  printf(ANSI_RESET);
}

void log_inf(const char *format, ...) {
  printf("%s ", LOG_PREFIX_INF);
  _log(format);
  printf(ANSI_RESET);
}

void log_wrn(const char *format, ...) {
  printf("%s ", LOG_PREFIX_WRN);
  _log(format);
  printf(ANSI_RESET);
}

void log_err(const char *format, ...) {
  printf("%s ", LOG_PREFIX_ERR);
  _log(format);
  printf(ANSI_RESET);
}

