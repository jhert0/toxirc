#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_FILE "toxirc.ini"

#include <stdbool.h>

#include <tox/tox.h>

struct Settings {
    char *name;
    char *status;
    bool ipv6;
    bool udp;
    char master[TOX_ADDRESS_SIZE * 2];
    char *default_channel;
};

typedef struct Settings SETTINGS;

extern SETTINGS settings;

void settings_save(char *file);

void settings_load(char *file);

#endif
