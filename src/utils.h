#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>

uint8_t *hex_string_to_bin(const char *hex_string);

off_t get_file_size(FILE *fp);

void to_hex(char *out, uint8_t *in, int size);

#endif
