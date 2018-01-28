#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include <tox/tox.h>

#define SAVE_FILE "toxirc_save.tox"

/*
 * Writes the tox save data to the specified path
 * returns true on success
 * returns false on failure
 */
bool save_write(Tox *tox, char *path);

/*
 * Loads the tox save data from the specified path
 * returns a Tox struct on success
 * returns NULL on failure
 */
Tox *save_load(char *path, int *status);

#endif
