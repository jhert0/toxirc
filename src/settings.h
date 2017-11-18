#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_FILE "toxirc.ini"

#include <stdbool.h>
#include <netdb.h>

#include <tox/tox.h>

#include "irc.h"

struct Settings {
    char name[TOX_MAX_NAME_LENGTH];
    char status[TOX_MAX_STATUS_MESSAGE_LENGTH];
    bool ipv6;
    bool udp;
    char master[TOX_ADDRESS_SIZE * 2];
    char server[NI_MAXHOST];
    char port[IRC_PORT_LENGTH];
    char default_channel[IRC_MAX_CHANNEL_LENGTH];
};

typedef struct Settings SETTINGS;

extern SETTINGS settings;

void settings_save(char *file);

bool settings_load(char *file);

#endif
