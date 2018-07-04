#include "commands.h"

#include "../irc.h"
#include "../logging.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <tox/tox.h>

size_t command_parse(char *msg, size_t msg_length){
    size_t cmd_length = 0;

    for (unsigned int i = 0; i < msg_length; i++) {
        if (msg[i] == ' ' || msg[i] == '\0') {
            cmd_length = i;
            break;
        }
    }

    return cmd_length;
}

char *command_parse_arg(char *msg, size_t msg_length, size_t cmd_length, int *arg_length){
    *arg_length = 0;

    if (cmd_length == 0 || (msg_length - 1) == cmd_length) {
        return NULL;
    }

    char *arg = strdup(msg + cmd_length + 1);
    if (!arg) {
        return NULL;
    }

    for (unsigned int i = cmd_length + 1; i < msg_length; i++) {
        if (msg[i] == ' ' || msg[i] == '\0') {
            *arg_length  = i - cmd_length - 1;
            arg[*arg_length] = '\0';
            break;
        }
    }

    return arg;
}

bool command_execute(const Command *cmds, char *msg, size_t msg_length){
    int arg_length;
    size_t cmd_length = command_parse(msg, msg_length);
    char *arg = command_parse_arg(msg, msg_length, cmd_length, &arg_length);

    bool valid = false;
    for (int i = 0; cmds[i].cmd; i++) {
        if (strncmp(msg, cmds[i].cmd, strlen(cmds[i].cmd)) == 0) {
            //cmds[i].func(tox, irc, channel_index, NULL);
            valid = true;
        }
    }

    if (arg) {
        free(arg);
    }

    return valid;
}
