#include "irc_callbacks.h"

#include "../irc.h"
#include "../settings.h"
#include "../tox.h"

#include "../commands/irc_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tox/tox.h>

static void message_callback(IRC *irc, char *buffer, void *arg) {
    Tox *tox = arg;

    char nick[32], user[32], server[100], channel[IRC_MAX_CHANNEL_LENGTH], msg[256];
    int  matches = sscanf(buffer, ":%31[^!]!%31[^@]@%99s PRIVMSG %49s :%255[^\r\n]", nick, user, server, channel, msg);
    if (matches != 5) {
        return;
    }

    // clang-format off
    char *nosync_prefix = settings_get_prefix(CHAR_NO_SYNC_PREFIX);
    if (command_prefix_cmp(msg, nosync_prefix)) { // dont sync messages that begin with ~
        return;
    }
    // clang-format on

    char *cmd_prefix = settings_get_prefix(CHAR_CMD_PREFIX);
    if (command_prefix_cmp(msg, cmd_prefix)) {
        size_t msg_length = strlen(msg);

        size_t cmd_length;
        char * cmd = command_parse(msg, msg_length, &cmd_length);
        if (!cmd) {
            return;
        }

        size_t arg_length;
        char * arg = command_parse_arg(msg, msg_length, cmd_length, &arg_length);

        for (int i = 0; irc_commands[i].cmd; i++) {
            if (strncmp(cmd, irc_commands[i].cmd, strlen(irc_commands[i].cmd)) == 0) {
                const uint32_t channel_index = irc_get_channel_index(irc, channel);
                irc_commands[i].func(tox, irc, channel_index, NULL);
            }
        }

        free(cmd);
        if (arg) {
            free(arg);
        }

        return; // dont sync commands
    }

    uint32_t group = irc_get_channel_group(irc, channel);
    if (group == UINT32_MAX) {
        return;
    }

    tox_group_send_msg(tox, group, nick, msg);
}

void irc_callbacks_setup(IRC *irc) {
    irc_set_message_callback(irc, message_callback);
}
