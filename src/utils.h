#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>

/*
 * Converts a hex string to binary
 * returns the binary string on success
 * returns NULL on failure
 */
uint8_t *hex_string_to_bin(const char *hex_string);

/*
 * Gets the specified file's size
 * returns the file size
 */
off_t get_file_size(char *file);

/*
 * Converts a string to hex
 */
void to_hex(char *out, uint8_t *in, int size);

#endif
