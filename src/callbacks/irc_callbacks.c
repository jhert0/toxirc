#include "irc_callbacks.h"

#include "../irc.h"
#include "../settings.h"
#include "../tox.h"
#include "../logging.h"
#include "../save.h"

#include "../commands/irc_commands.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tox/tox.h>

static void message_callback(IRC *irc, char *buffer, void *arg) {
    Tox *tox = arg;

    irc_message *message = irc_parse_message(buffer);
    if (!message) {
        DEBUG("IRC Callbacks", "Could not parse message");
        return;
    }

    if (message->type == IRC_MESSGAE_PRIVMSG) {
        char *nosync_prefix = settings_get_prefix(CHAR_NO_SYNC_PREFIX);
        if (command_prefix_cmp(message->message, nosync_prefix)) { // dont sync messages that begin with ~
            free(message);
            return;
        }

        char *cmd_prefix = settings_get_prefix(CHAR_CMD_PREFIX);
        if (command_prefix_cmp(message->message, cmd_prefix)) {
            size_t msg_length = strlen(message->message);

            size_t cmd_length;
            char * cmd = command_parse(message->message, msg_length, &cmd_length);
            if (!cmd) {
                free(message);
                return;
            }

            size_t arg_length;
            char * arg = command_parse_arg(message->message, msg_length, cmd_length, &arg_length);

            for (int i = 0; irc_commands[i].cmd; i++) {
                if (strncmp(cmd, irc_commands[i].cmd, strlen(irc_commands[i].cmd)) == 0) {
                    const uint32_t channel_index = irc_get_channel_index(irc, message->channel);
                    irc_commands[i].func(tox, irc, channel_index, NULL);
                }
            }

            free(cmd);
            if (arg) {
                free(arg);
            }

            free(message);

            return; // dont sync commands
        }

        uint32_t group = irc_get_channel_group(irc, message->channel);
        if (group == UINT32_MAX) {
            free(message);
            return;
        }

        tox_group_send_msg(tox, group, message->nick, message->message);
    } else {
        DEBUG("IRC Callbacks", "Recieved reply with code: %d for channel: %s", message->code, message->channel);

        if (message->code == 403) { // channel doesn't exist
            uint32_t index = irc_get_channel_index(irc, message->channel);
            if (index == UINT32_MAX) {
                DEBUG("IRC Callbacks", "Got message for channel: %s but we have no record of being in it.",
                      message->channel);
                free(message);
                return;
            }

            DEBUG("IRC Callbacks", "Channel does not exist leaving...");
            tox_conference_delete(tox, irc->channels[index].group_num, NULL);
            irc_delete_channel(irc, index); // only delete the channel, we don't need to leave since we never joined

            save_write(tox, SAVE_FILE);
        }
    }

    free(message);
}

void irc_callbacks_setup(IRC *irc) {
    irc_set_message_callback(irc, message_callback);
}
