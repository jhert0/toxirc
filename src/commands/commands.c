#include "commands.h"

#include "../irc.h"
#include "../logging.h"
#include "../settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <tox/tox.h>

char *command_parse(char *msg, size_t msg_length, size_t *cmd_length) {
    char *cmd   = NULL;
    *cmd_length = 0;

    char * cmd_prefix    = settings_get_prefix(CHAR_CMD_PREFIX);
    size_t prefix_length = strlen(cmd_prefix);
    if (strncmp(msg, cmd_prefix, prefix_length) == 0) {
        msg += prefix_length; // get rid of the command prefix
    }

    for (size_t i = 0; i < msg_length; i++) {
        if (msg[i] == ' ' || msg[i] == '\0') {
            *cmd_length = i;
            cmd         = malloc(*cmd_length);
            if (!cmd) {
                *cmd_length = 0;
                return NULL;
            }

            memcpy(cmd, msg, *cmd_length);
            break;
        }
    }

    return cmd;
}

char *command_parse_arg(char *msg, size_t msg_length, size_t cmd_length, size_t *arg_length) {
    *arg_length = 0;

    if ((msg_length - 1) == cmd_length) { // no arguments
        return NULL;
    }

    char *arg = strdup(msg + cmd_length + 1);
    if (!arg) {
        return NULL;
    }

    for (size_t i = cmd_length + 1; i < msg_length; i++) {
        if (msg[i] == ' ' || msg[i] == '\0') {
            *arg_length      = i - cmd_length - 1;
            arg[*arg_length] = '\0';
            break;
        }
    }

    return arg;
}

bool command_prefix_cmp(char *line, char *prefix) {
    return strncmp(line, prefix, strlen(prefix)) == 0;
}
