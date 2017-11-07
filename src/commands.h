#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

#include <tox/tox.h>

#include "irc.h"

struct Command {
    char *cmd;
    char *desc;
    bool master;
    bool (*func)(Tox *tox, IRC *irc, int fid, char *arg);
};

struct Command commands[256];

size_t command_parse(char *msg, size_t msg_length);

char *command_parse_arg(char *msg, size_t msg_length, size_t cmd_length, int *arg_length);

#endif
