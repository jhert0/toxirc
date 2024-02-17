#include "group_commands.h"

#include "commands.h"

#include "../irc.h"
#include "../macros.h"
#include "../settings.h"

#include <stdio.h>
#include <string.h>
#include <tox/tox.h>

static bool command_channel(Tox *tox, IRC *irc, uint32_t group_num, char *arg);
static bool command_characters(Tox *tox, IRC *irc, uint32_t group_num, char *arg);
static bool command_help(Tox *tox, IRC *irc, uint32_t group_num, char *arg);

// clang-format off
struct Command group_commands[MAX_CMDS] = {
    { "channel",    "Gets the channel this groupchat is being synced with.",              false, command_channel    },
    { "characters", "Gives information about all the special characters.",                false, command_characters },
    { "help",       "This command.",                                                      false, command_help       },
    { NULL,         NULL,                                                                 false, NULL               },
};
// clang-format on

static bool command_channel(Tox *tox, IRC *irc, uint32_t group_num, char *UNUSED(arg)) {
    char *channel_name = irc_get_channel_by_group(irc, group_num);

    char buffer[TOX_MAX_MESSAGE_LENGTH];
    int  length = snprintf(buffer, sizeof(buffer), "This groupchat is being synced with %s", channel_name);

    tox_group_send_message(tox, group_num, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)buffer, length, NULL);

    return true;
}

static bool command_help(Tox *tox, IRC *UNUSED(irc), uint32_t group_num, char *UNUSED(arg)) {
    char *cmd_prefix = settings_get_prefix(CHAR_CMD_PREFIX);
    for (int i = 0; group_commands[i].cmd; i++) {
        char   message[TOX_MAX_MESSAGE_LENGTH];
        size_t length =
            snprintf(message, sizeof(message), "%s%s: %s", cmd_prefix, group_commands[i].cmd, group_commands[i].desc);
        tox_group_send_message(tox, group_num, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)message, length, NULL);
    }
    return true;
}

static bool command_characters(Tox *tox, IRC *UNUSED(irc), uint32_t group_num, char *UNUSED(arg)) {
    for (unsigned int i = 0; i < CHAR_MAX; i++) {
        char   message[TOX_MAX_MESSAGE_LENGTH];
        size_t length = sprintf(message, "%s: %s", settings.characters[i].prefix, settings.characters[i].desc);
        tox_group_send_message(tox, group_num, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)message, length, NULL);
    }

    return true;
}
