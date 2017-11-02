#include "tox.h"

#include "irc.h"
#include "logging.h"
#include "macros.h"
#include "save.h"
#include "utils.h"

#include <tox/tox.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

//TODO: Create an array of nodes to bootsrap from
char *bootstrap_ip = "46.101.197.175";
char *bootstrap_address = "CD133B521159541FB1D326DE9850F5E56A6C724B5B8E5EB5CD8D950408E95707";
uint16_t bootstrap_port = 443;

static void friend_message_callback(Tox *tox, uint32_t fid, TOX_MESSAGE_TYPE type, const uint8_t *message,
                                    size_t UNUSED(length), void *userdata){
    if (type != TOX_MESSAGE_TYPE_NORMAL) {
        return;
    }

    int group_num = 0;
    IRC *irc = userdata;
    int index;
    switch ((char)message[0]) {
        case 'i':
            tox_conference_invite(tox, fid, group_num, NULL);
            break;
        case 'h': //TODO: implement help command
            break;
        case 'j': //TODO: Parse channel name
            irc_join_channel(irc, "#synbot_test");
            TOX_ERR_CONFERENCE_NEW err;
            irc->channels[irc->num_channels - 1].group_num = tox_conference_new(tox, &err);
            if (irc->channels[irc->num_channels - 1].group_num == UINT32_MAX) {
                DEBUG("Tox", "Could not create groupchat. Error number: %d", err);
            }
            break;
        case 'd'://TODO: Parse channel name
            index = irc_get_channel_index(irc, "#syncbot_test");
            if (index == -1) {
                DEBUG("Tox", "Could not get irc channel index.");
                return;
            }

            irc_leave_channel(irc, index);
            tox_conference_delete(tox, irc->channels[index].group_num, NULL);
            break;
        case 'l':
            for (int i = 0; i < irc->num_channels; i++) {
                if (irc->channels[i].in_channel){
                    tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const unsigned char *)irc->channels[i].name, strlen(irc->channels[i].name), NULL);
                }
            }
            break;
        default:
            tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const uint8_t *)"Unknown command.", sizeof("Unknown command.") - 1, NULL);
            break;
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
        memcpy(name, "Unknown", 7);
        name_len = 7;
    } else {
        tox_conference_peer_get_name(tox, groupnumber, peer_number, name, NULL);
    }

    IRC *irc = userdata;
    char *channel = irc_get_channel_by_group(irc, groupnumber);
    if (!channel) {
        DEBUG("Tox", "Could not get channel name, unable to send message.");
        return;
    }

    irc_message(irc->sock, channel, (char *)name, (char *)message);

}

static void friend_request_callback(Tox *tox, const uint8_t *public_key, const uint8_t *UNUSED(data),
                                    size_t UNUSED(length), void *UNUSED(userdata)){
    DEBUG("Tox", "Received a friend request.");

    TOX_ERR_FRIEND_ADD err;
    tox_friend_add_norequest(tox, public_key, &err);

    if (err != TOX_ERR_FRIEND_ADD_OK) {
        DEBUG("Tox", "Error sending friend request. Error number: %d", err);
    }

    write_config(tox, SAVE_FILE);

    DEBUG("Tox", "Add friend: %s.", public_key);
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
    Tox *tox = load_config(SAVE_FILE, &status);

    DEBUG("Tox", "Initialzing tox");

    if (!tox) {
        DEBUG("Tox", "Could not create tox instance.");
        return NULL;
    }

    if (!tox_connect(tox)) {
        return NULL;
    }

    tox_self_set_name(tox, (const uint8_t *)NAME, sizeof(NAME) - 1, NULL);
    tox_self_set_status_message(tox, (const uint8_t *)STATUS, sizeof(STATUS) - 1, NULL);

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
        write_config(tox, SAVE_FILE);
    }

    DEBUG("Tox", "Finished initialzing tox");

    return tox;
}

bool tox_connect(Tox *tox){
    uint8_t *key = hex_string_to_bin(bootstrap_address);
    if (!key) {
        DEBUG("Tox", "Could not allocate memory for key.");
        return false;
    }

    TOX_ERR_BOOTSTRAP err;
    tox_bootstrap(tox, bootstrap_ip, bootstrap_port, key, &err);

    if (err != TOX_ERR_BOOTSTRAP_OK) {
        DEBUG("Tox", "Could not bootstrap with: ip: %s port: %d key: %s", bootstrap_ip, bootstrap_port, key);
        free(key);
        return false;
    }

    free(key);
    DEBUG("Tox", "Connected to tox network.");
    return true;
}

void tox_group_send_msg(Tox *tox, uint32_t group_num, char *nick, char *msg){
    size_t size = strlen(nick) + strlen(msg) + 3;
    char *message = malloc(size);
    if (!message) {
        DEBUG("Tox", "Could not allocate memory for group message.");
        return;
    }

    int message_length = snprintf(message, size, "%s: %s", nick, msg);
    tox_conference_send_message(tox, group_num, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)message, message_length, NULL);
    free(message);
}
