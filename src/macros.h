#ifndef MACROS_H
#define MACROS_H

#define UNUSED(x) UNUSED_##x __attribute__((__unused__))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#endif
