#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

#include <tox/tox.h>

#include "irc.h"

struct Command {
    char *cmd;
    char *desc;
    bool (*func)(Tox *tox, IRC *irc, int fid, char *arg);
};

struct Command commands[256];

int command_get_length(char *msg, size_t msg_length);

char *command_get_arg(char *msg, size_t msg_length, int cmd_length, int *arg_length);

#endif
