#include "tox_callbacks.h"

#include "../irc.h"
#include "../macros.h"
#include "../logging.h"
#include "../save.h"

#include "../commands/group_commands.h"
#include "../commands/friend_commands.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <tox/tox.h>

static void friend_message_callback(Tox *tox, uint32_t fid, TOX_MESSAGE_TYPE type, const uint8_t *message,
                                    size_t length, void *userdata){
    if (type != TOX_MESSAGE_TYPE_NORMAL) {
        return;
    }

    IRC *irc = userdata;

    char msg[TOX_MAX_MESSAGE_LENGTH];
    length = MIN(TOX_MAX_MESSAGE_LENGTH - 1, length);
    memcpy(msg, message, length);
    msg[length] = '\0';

    length++;

    int arg_length;
    size_t cmd_length = command_parse(msg, length);
    char *arg = command_parse_arg(msg, length, cmd_length, &arg_length);

    bool valid = false;
    for (int i = 0; friend_commands[i].cmd; i++) {
        if (strncmp(msg, friend_commands[i].cmd, strlen(friend_commands[i].cmd)) == 0) {
            friend_commands[i].func(tox, irc, fid, arg);
            valid = true;
        }
    }

    if (arg) {
        free(arg);
    }

    if (!valid) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL,
                                (uint8_t *)"Invalid command send me help to find out what commands I support",
                                sizeof("Invalid command send me help to find out what commands I support") - 1, NULL);
    }
}

static void group_message_callback(Tox *tox, uint32_t groupnumber,
                                   uint32_t peer_number, TOX_MESSAGE_TYPE UNUSED(type),
                                   const uint8_t *message, size_t length, void *userdata){

    if (tox_conference_peer_number_is_ours(tox, groupnumber, peer_number, NULL)) {
        return;
    }

    IRC *irc = userdata;

    if (message[0] == '~'){ //messages beggining with ~ are not synced with irc
        return;
    } else if (message[0] == '!') {
        char msg[TOX_MAX_MESSAGE_LENGTH];
        length = MIN(TOX_MAX_MESSAGE_LENGTH - 1, length);
        memcpy(msg, message, length);
        msg[length] = '\0';

        length++;

        int arg_length;
        size_t cmd_length = command_parse(msg, length);
        char *arg = command_parse_arg(msg, length, cmd_length, &arg_length);

        bool valid = false;
        for (int i = 0; group_commands[i].cmd; i++) {
            if (strncmp(msg, group_commands[i].cmd, strlen(group_commands[i].cmd)) == 0) {
                group_commands[i].func(tox, irc, groupnumber, NULL);
                valid = true;
            }
        }

        if (arg) {
            free(arg);
        }

        if (!valid) {
            tox_conference_send_message(tox, groupnumber, TOX_MESSAGE_TYPE_NORMAL,
                                        (uint8_t *)"Invalid command send me help to find out what commands I support",
                                        sizeof("Invalid command send me help to find out what commands I support") - 1, NULL);
        }

        return;
    }

    uint8_t name[TOX_MAX_NAME_LENGTH];
    TOX_ERR_CONFERENCE_PEER_QUERY err;
    int name_len = tox_conference_peer_get_name_size(tox, groupnumber, peer_number, &err);

    if (name_len == 0 || err != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
        memcpy(name, "unknown", 7);
        name_len = 7;
    } else {
        tox_conference_peer_get_name(tox, groupnumber, peer_number, name, NULL);
    }

    name[name_len] = '\0';

    char *channel = irc_get_channel_by_group(irc, groupnumber);
    if (!channel) {
        DEBUG("Tox", "Could not get channel name, unable to send message.");
        return;
    }

    char buffer[name_len + length + 3];
    sprintf(buffer, "<%s> %s", name, message);

    irc_message(irc, channel, buffer);
}

static void friend_request_callback(Tox *tox, const uint8_t *public_key, const uint8_t *UNUSED(data),
                                    size_t UNUSED(length), void *UNUSED(userdata)){
    TOX_ERR_FRIEND_ADD err;
    tox_friend_add_norequest(tox, public_key, &err);

    if (err != TOX_ERR_FRIEND_ADD_OK) {
        DEBUG("Tox", "Error accepting friend request. Error number: %d", err);
        return;
    }

    save_write(tox, SAVE_FILE);
}

static void self_connection_change_callback(Tox *UNUSED(tox), TOX_CONNECTION status, void *UNUSED(userdata)) {
    switch (status) {
        case TOX_CONNECTION_NONE:
            DEBUG("Tox", "Lost connection to the Tox network.");
            break;
        case TOX_CONNECTION_TCP:
            DEBUG("Tox", "Connected using TCP.");
            break;
        case TOX_CONNECTION_UDP:
            DEBUG("Tox", "Connected using UDP.");
            break;
    }
}

void tox_callbacks_setup(Tox *tox){
    tox_callback_self_connection_status(tox, &self_connection_change_callback);
    tox_callback_friend_message(tox, &friend_message_callback);
    tox_callback_friend_request(tox, &friend_request_callback);
    tox_callback_conference_message(tox, &group_message_callback);
}
