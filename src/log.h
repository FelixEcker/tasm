#ifndef LOG_H
#define LOG_H

#define ESC "\x1b"
#define ANSI_RESET ESC "[0m"
#define ANSI_GRAY ESC "[90m"
#define ANSI_GREEN ESC "[32m"
#define ANSI_ORANGE ESC "[33m"
#define ANSI_RED ESC "[31m"

#define LOG_PREFIX_DBG ANSI_GRAY "[*]"
#define LOG_PREFIX_INF ANSI_GREEN "[i]"
#define LOG_PREFIX_WRN ANSI_ORANGE "[w]"
#define LOG_PREFIX_ERR ANSI_RED "[e]"

void log_dbg(const char *format, ...);
void log_inf(const char *format, ...);
void log_wrn(const char *format, ...);
void log_err(const char *format, ...);

#endif
