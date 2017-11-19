#include "logging.h"

#include "settings.h"

#include <stdio.h>
#include <stdarg.h>

void debug(const char *fmt, ...){
    if (!settings.debug_messages) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
