#include "logging.h"

#include <stdio.h>
#include <stdarg.h>

void debug(const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
