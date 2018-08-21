#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

#include <tox/tox.h>

#include "../irc.h"

#define MAX_CMDS 256

struct Command {
    char *cmd;
    char *desc;
    bool master;
    bool (*func)(Tox *tox, IRC *irc, uint32_t index, char *arg); //index is either the friend number, group number, or channel index
};

typedef struct Command Command;

char *command_parse(char *msg, size_t msg_length, size_t *cmd_length);

char *command_parse_arg(char *msg, size_t msg_length, size_t cmd_length, size_t *arg_length);

bool command_prefix_cmp(char *line, char *prefix);

#endif
