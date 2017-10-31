#ifndef LOGGING_H
#define LOGGING_H

#define DEBUG(file, str, ...) debug("%-14s" str "\n", file ": ", ## __VA_ARGS__)

void debug(const char *fmt, ...);

#endif
