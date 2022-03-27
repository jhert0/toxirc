#ifndef BOT_H
#define BOT_H

#include <stdint.h>
#include <tox/tox.h>
#include <time.h>
#include "irc.h"

struct Bot {
    Tox *tox;

    IRC **   irc;
    uint32_t num_servers;
    uint32_t server_size;

    time_t started;
};

typedef struct Bot Bot;

extern Bot bot;

bool bot_init();

bool bot_add_irc_server(IRC *irc);

void bot_remove_irc_server(uint32_t index);

void bot_kill();

#endif
