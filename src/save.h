#ifndef SAVE_H
#define SAVE_H

#include <stdint.h>
#include <tox/tox.h>

#define SAVE_FILE "syncbot_save"

bool write_config(Tox *tox, char *path);

Tox *load_config(char *path, int *status);

#endif
