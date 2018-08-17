#include "irc_callbacks.h"

#include "../irc.h"
#include "../settings.h"
#include "../tox.h"

#include "../commands/irc_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tox/tox.h>

static void message_callback(IRC *irc, char *buffer, void *arg){
    Tox *tox = arg;

    char nick[32], user[32], server[100], channel[IRC_MAX_CHANNEL_LENGTH], msg[256];
    int matches = sscanf(buffer, ":%31[^!]!%31[^@]@%99s PRIVMSG %49s :%255[^\r\n]", nick, user, server, channel, msg);
    if (matches != 5) {
        return;
    }

    if (strncmp(msg, settings.characters[CHAR_NO_SYNC_PREFIX].prefix, strlen(settings.characters[CHAR_NO_SYNC_PREFIX].prefix)) == 0) { //dont sync messages that begin with ~
        return;
    } else if (strncmp(msg, settings.characters[CHAR_CMD_PREFIX].prefix, strlen(settings.characters[CHAR_CMD_PREFIX].prefix)) == 0) {
        size_t msg_length = strlen(msg);

        size_t cmd_length;
        char *cmd = command_parse(msg, msg_length, &cmd_length);
        if (!cmd) {
            return;
        }

        size_t arg_length;
        char *arg = command_parse_arg(msg, msg_length, cmd_length, &arg_length);

        bool valid = false;
        for (int i = 0; irc_commands[i].cmd; i++) {
            if (strncmp(cmd, irc_commands[i].cmd, strlen(irc_commands[i].cmd)) == 0) {
                const uint32_t channel_index = irc_get_channel_index(irc, channel);
                irc_commands[i].func(tox, irc, channel_index, NULL);
                valid = true;
            }
        }

        free(cmd);
        if (arg) {
            free(arg);
        }

        /*
        if (!valid) {
            irc_message(irc, channel, "Invalid command.");
        }
        */

        return; //dont sync commands
    }

    uint32_t group = irc_get_channel_group(irc, channel);
    if (group == UINT32_MAX) {
        return;
    }

    tox_group_send_msg(tox, group, nick, msg);
}

void irc_callbacks_setup(IRC *irc){
    irc_set_message_callback(irc, message_callback);
}
