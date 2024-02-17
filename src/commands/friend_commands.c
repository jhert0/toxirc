#include "friend_commands.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tox/tox.h>

#include "commands.h"

#include "../bot.h"
#include "../logging.h"
#include "../macros.h"
#include "../settings.h"
#include "../tox.h"
#include "../utils.h"
#include "../save.h"

static bool command_invite(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_join(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_leave(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_list(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_id(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_info(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_la(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_name(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_default(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_master(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_help(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_warn(Tox *tox, IRC *irc, uint32_t fid, char *arg);
static bool command_limit(Tox *tox, IRC *irc, uint32_t fid, char *arg);

// clang-format off
struct Command friend_commands[MAX_CMDS] = {
    { "invite",  "invite #channelname to get invited to a channel the bot has joined.",   false, command_invite  },
    { "join",    "join #channelname to join a specific channel.",                         false, command_join    },
    { "leave",   "leave #channelname to leave a specific channel.",                       true,  command_leave   },
    { "list",    "Shows all channels the bot has joined.",                                false, command_list    },
    { "id",      "Displays this bot's tox ID.",                                           false, command_id      },
    { "info",    "Shows additional info about this bot.",                                 false, command_info    },
    { "la",      "Forces this bot to leave all channels",                                 true,  command_la      },
    { "name",    "Set this bot's name",                                                   true,  command_name    },
    { "default", "default #channelname sets the default channel for invite.",             true,  command_default },
    { "master",  "Add a ToxID to set the bot's owner.",                                   true,  command_master  },
    { "warn",    "Warn all channels and groupchats the bot is going down.",               true,  command_warn    },
    { "limit",   "Limit the number of channels the bot can join.",                        true,  command_limit   },
    { "help",    "Displays this list of commands.",                                       false, command_help    },
    { NULL,      NULL,                                                                    false, NULL            },
};
// clang-format on

static bool command_invite(Tox *tox, IRC *irc, uint32_t fid, char *arg) {
    uint32_t index;

    if (!arg) {
        index = irc_get_channel_index(irc, settings.default_channel);
    } else {
        index = irc_get_channel_index(irc, arg);
    }

    if (index == UINT32_MAX || !irc->channels[index].in_channel) {
        return false;
    }

    tox_group_invite_friend(tox, irc->channels[index].group_num, fid, NULL);

    return true;
}

static bool command_join(Tox *tox, IRC *irc, uint32_t fid, char *arg) {
    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.",
                                sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    if ((irc->num_channels + 1) >= settings.channel_limit) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"Channel Limit reached.",
                                sizeof("Channel limit reached.") - 1, NULL);
        return false;
    }

    if (strlen(arg) > IRC_MAX_CHANNEL_LENGTH) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"IRC channel name is too long",
                                sizeof("IRC channel name is too long") - 1, NULL);
        return false;
    }

    uint32_t index = irc_get_channel_index(irc, arg);
    if (index != UINT32_MAX) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"I am already in that channel.",
                                sizeof("I am already in that channel.") - 1, NULL);
        return false;
    }

    Tox_Err_Group_New err;
    uint32_t          group_num = tox_group_new(tox, TOX_GROUP_PRIVACY_STATE_PUBLIC, (uint8_t *)arg, strlen(arg),
                                                (uint8_t *)settings.name, strlen(settings.name), &err);
    if (group_num == UINT32_MAX) {
        DEBUG("Tox", "Could not create groupchat. Error number: %d", err);
        return false;
    }

    irc_join_channel(irc, arg, group_num);

    tox_group_set_topic(tox, group_num, (uint8_t *)arg, strlen(arg), NULL);
    tox_group_invite_friend(tox, group_num, fid, NULL);

    save_write(tox, SAVE_FILE);

    return true;
}

static bool command_leave(Tox *tox, IRC *irc, uint32_t fid, char *arg) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.",
                                sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    uint32_t index = irc_get_channel_index(irc, arg);
    if (index == UINT32_MAX) {
        DEBUG("Commands", "Could not get irc channel index for: %s.", arg);
        return false;
    }

    tox_group_leave(tox, index, (uint8_t *)"no longer syncing...", sizeof("no longer syncing...") - 1, NULL);
    irc_leave_channel(irc, index);

    save_write(tox, SAVE_FILE);

    return true;
}

static bool command_list(Tox *tox, IRC *irc, uint32_t fid, char *UNUSED(arg)) {
    if (irc->num_channels == 0) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"I'm currently in no channels.",
                                strlen("I'm currently in no channels.") - 1, NULL);
        return true;
    }

    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].in_channel) {
            tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)irc->channels[i].name,
                                    strlen(irc->channels[i].name), NULL);
        }
    }

    return true;
}

static bool command_id(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *UNUSED(arg)) {
    uint8_t public_key_bin[TOX_ADDRESS_SIZE];
    char    public_key_str[TOX_ADDRESS_SIZE * 2];

    tox_self_get_address(tox, public_key_bin);
    to_hex(public_key_str, public_key_bin, TOX_ADDRESS_SIZE);

    tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)public_key_str, TOX_ADDRESS_SIZE * 2, NULL);

    return true;
}

static bool command_help(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *UNUSED(arg)) {
    for (int i = 0; friend_commands[i].cmd; i++) {
        if (!friend_commands[i].master || (friend_commands[i].master && tox_is_friend_master(tox, fid))) {
            char   message[TOX_MAX_MESSAGE_LENGTH] = { 0 };
            size_t length =
                snprintf(message, sizeof(message), "%s: %s", friend_commands[i].cmd, friend_commands[i].desc);
            tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)message, length, NULL);
        }
    }

    return true;
}

static bool command_info(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *UNUSED(arg)) {
    int num_frends = tox_self_get_friend_list_size(tox);

    uint32_t friends[num_frends];
    tox_self_get_friend_list(tox, friends);

    int online = 0;
    for (int i = 0; i < num_frends; i++) {
        if (tox_friend_get_connection_status(tox, i, NULL) != TOX_CONNECTION_NONE) {
            online++;
        }
    }

    char time_str[15];
    strftime(time_str, 15, "%m/%d/%Y", localtime(&bot.started));

    char message[TOX_MAX_MESSAGE_LENGTH];
    int  length = snprintf(message, sizeof(message), "I am friends with %d people, %d of them are online. Started %s.",
                           num_frends, online, time_str);

    tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)message, length, NULL);

    return true;
}

static bool command_la(Tox *tox, IRC *irc, uint32_t fid, char *UNUSED(arg)) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    for (uint32_t i = 0; i < irc->num_channels; i++) {
        tox_group_leave(tox, irc->channels[i].group_num, (uint8_t *)"no longer syncing...",
                        sizeof("no longer syncing...") - 1, NULL);
    }

    irc_leave_all_channels(irc);

    save_write(tox, SAVE_FILE);

    return true;
}

static bool command_name(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *arg) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.",
                                sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    strcpy(settings.name, arg);

    tox_self_set_name(tox, (const uint8_t *)arg, strlen(arg), NULL);

    save_write(tox, SAVE_FILE);
    settings_save(SETTINGS_FILE);

    return true;
}

static bool command_default(Tox *tox, IRC *irc, uint32_t fid, char *arg) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.",
                                sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    if (!irc_in_channel(irc, arg)) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"I am not in that channel.",
                                sizeof("I am not in that channel.") - 1, NULL);
        return false;
    }

    strcpy(settings.default_channel, arg);

    settings_save(SETTINGS_FILE);

    return true;
}

static bool command_master(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *arg) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.",
                                sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    strcpy(settings.master, arg);

    settings_save(SETTINGS_FILE);

    return true;
}

static bool command_warn(Tox *tox, IRC *irc, uint32_t fid, char *UNUSED(arg)) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    char   warning[TOX_MAX_MESSAGE_LENGTH]; // TODO: allow the message to be passed as an argument
    size_t length =
        snprintf(warning, sizeof(warning), "%s is about to be shutdown. Sorry for the inconvenience.", settings.name);

    for (uint32_t i = 0; i < irc->num_channels; i++) {
        irc_send_message(irc, irc->channels[i].name, warning);
        tox_group_send_message(tox, irc->channels[i].group_num, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)warning,
                               length, NULL);
    }

    return true;
}

static bool command_limit(Tox *tox, IRC *UNUSED(irc), uint32_t fid, char *arg) {
    if (!tox_is_friend_master(tox, fid)) {
        return false;
    }

    long new_limit = atol(arg);
    if (new_limit == -1) {
        new_limit = UINT32_MAX;
    }

    settings.channel_limit = new_limit;

    return true;
}
