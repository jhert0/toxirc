#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_FILE "settings.ini"

#include <stdbool.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif

#include <tox/tox.h>

#include "irc.h"

enum prefix_index {
    CHAR_CMD_PREFIX,
    CHAR_NO_SYNC_PREFIX,
    CHAR_MAX,
};

#define MAX_PREFIX 3

struct special_characters {
    char  prefix[MAX_PREFIX + 1];
    char *desc;
};

typedef struct special_characters SpecialCharacters;

#define MASTER_KEY_SIZE (TOX_ADDRESS_SIZE * 2)

struct Settings {
    // Bot
    char              name[TOX_MAX_NAME_LENGTH];
    char              master[MASTER_KEY_SIZE + 1];
    bool              verbose;
    SpecialCharacters characters[CHAR_MAX];

    // Tox
    char status[TOX_MAX_STATUS_MESSAGE_LENGTH];
    bool ipv6;
    bool udp;

    // IRC
    char     server[NI_MAXHOST];
    char     port[IRC_PORT_LENGTH];
    char     default_channel[IRC_MAX_CHANNEL_LENGTH];
    uint32_t channel_limit;
    char     password[IRC_MAX_PASSWORD_LENGTH];
};

typedef struct Settings SETTINGS;

extern SETTINGS settings;

void settings_save(char *file);

bool settings_load(char *file);

char *settings_get_prefix(enum prefix_index index);

#endif
