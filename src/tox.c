#include "tox.h"

#include "commands.h"
#include "irc.h"
#include "logging.h"
#include "macros.h"
#include "nodes.h"
#include "save.h"
#include "settings.h"
#include "utils.h"

#include <tox/tox.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

    size_t cmd_length = command_parse(msg, length);

    int arg_length;
    char *arg = command_parse_arg(msg, length, cmd_length, &arg_length);

    bool valid = false;
    for (int i = 0; commands[i].cmd; i++) {
        if (strncmp(msg, commands[i].cmd, strlen(commands[i].cmd)) == 0) {
            commands[i].func(tox, irc, fid, arg);
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
                                   const uint8_t *message, size_t UNUSED(length), void *userdata){

    if (tox_conference_peer_number_is_ours(tox, groupnumber, peer_number, NULL)) {
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

    IRC *irc = userdata;
    char *channel = irc_get_channel_by_group(irc, groupnumber);
    if (!channel) {
        DEBUG("Tox", "Could not get channel name, unable to send message.");
        return;
    }

    irc_message(irc, channel, (char *)name, (char *)message);
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

Tox *tox_init(){
    int status;
    Tox *tox = save_load(SAVE_FILE, &status);

    DEBUG("Tox", "Initialzing tox");

    if (!tox) {
        DEBUG("Tox", "Could not create tox instance.");
        return NULL;
    }

    if (!tox_connect(tox)) {
        return NULL;
    }

    tox_self_set_name(tox, (const uint8_t *)settings.name, strlen(settings.name), NULL);
    tox_self_set_status_message(tox, (const uint8_t *)settings.status, strlen(settings.status), NULL);

    tox_callback_self_connection_status(tox, &self_connection_change_callback);
    tox_callback_friend_message(tox, &friend_message_callback);
    tox_callback_friend_request(tox, &friend_request_callback);
    tox_callback_conference_message(tox, &group_message_callback);

    uint8_t public_key_bin[TOX_ADDRESS_SIZE];
    char public_key_str[TOX_ADDRESS_SIZE * 2];

    tox_self_get_address(tox, public_key_bin);
    to_hex(public_key_str, public_key_bin, TOX_ADDRESS_SIZE);
    DEBUG("Tox", "ID: %.*s", TOX_ADDRESS_SIZE * 2, public_key_str);

    if (status == 2) {
        save_write(tox, SAVE_FILE);
    }

    DEBUG("Tox", "Finished initialzing tox");

    return tox;
}

bool tox_connect(Tox *tox){
    for (int i = 0; nodes[i].ip; i++) {
        uint8_t *key = hex_string_to_bin(nodes[i].key);
        if (!key) {
            DEBUG("Tox", "Could not allocate memory for key.");
            return false; //Return because it will most likely fail again
        }

        tox_bootstrap(tox, nodes[i].ip, nodes[i].udp_port, key, NULL);
        tox_add_tcp_relay(tox, nodes[i].ip, nodes[i].tcp_port, key, NULL);
    }

    DEBUG("Tox", "Connected to tox network.");

    return true;
}

void tox_group_send_msg(Tox *tox, uint32_t group_num, char *nick, char *msg){
    char message[TOX_MAX_MESSAGE_LENGTH];
    size_t length = snprintf(message, sizeof(message), "<%s> %s", nick, msg);
    tox_conference_send_message(tox, group_num, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)message, length, NULL);
}

bool tox_is_friend_master(Tox *tox, int fid){
    uint8_t public_key_bin[TOX_ADDRESS_SIZE];
    if (tox_friend_get_public_key(tox, fid, public_key_bin, NULL) == 0) {
        return false;
    }

    uint8_t *key = hex_string_to_bin(settings.master);
    if (!key) {
        return false;
    }

    if (strncmp((char *)public_key_bin, (char *)key, TOX_PUBLIC_KEY_SIZE) != 0) {
        free(key);
        return false;
    }

    free(key);

    return true;
}
