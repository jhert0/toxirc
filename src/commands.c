#include "commands.h"

#include "irc.h"
#include "logging.h"
#include "macros.h"
#include "utils.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <tox/tox.h>


int command_get_length(char *msg, size_t msg_length){
    int cmd_length = 0;

    for (unsigned int i = 0; i < msg_length; i++) {
        if (msg[i] == ' ' || (msg_length - 1) == i) {
            cmd_length = i;
            break;
        }
    }

    return cmd_length;
}

char *command_get_arg(char *msg, size_t msg_length, int cmd_length, int *arg_length){
    *arg_length = 0;

    if (cmd_length == 0) {
        return NULL;
    }

    char *arg = strdup(msg + cmd_length);
    if (!arg) {
        return NULL;
    }

    for (unsigned int i = 0; i < msg_length - cmd_length; i++) {
        if (msg[i] == ' ' || (msg_length - 1) == i) {
            *arg_length  = i;
            break;
        }
    }

    return arg;
}


static bool command_invite(Tox *tox, IRC *irc, int fid, char *arg);
static bool command_join(Tox *tox, IRC *irc, int fid, char *arg);
static bool command_leave(Tox *tox, IRC *irc, int fid, char *arg);
static bool command_list(Tox *tox, IRC *irc, int fid, char *arg);
static bool command_id(Tox *tox, IRC *irc, int fid, char *arg);
static bool command_help(Tox *tox, IRC *irc, int fid, char *arg);

struct Command commands[256] = {
    { "invite", "Request an invite to the default channel or specify one.", command_invite },
    { "join", "Joins the specified channel.", command_join },
    { "leave", "Leaves the specified channel.", command_leave },
    { "list", "List all channels I am in.", command_list },
    { "id", "Prints my tox ID.", command_id },
    { "help", "This message.", command_help },
    { NULL, NULL, NULL },
};

static bool command_invite(Tox *tox, IRC *irc, int fid, char *arg){
    int index;

    if (!arg) {
        index = irc_get_channel_index(irc, "#toxirc");
    } else {
        index = irc_get_channel_index(irc, arg);
    }

    if (index == -1 || !irc->channels[index].in_channel) {
        DEBUG("Commands", "error getting channel");
        return false;
    }

    tox_conference_invite(tox, fid, irc->channels[index].group_num, NULL);

    return true;
}

static bool command_join(Tox *tox, IRC *irc, int fid, char *arg){
    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.", sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    irc_join_channel(irc, arg);

    TOX_ERR_CONFERENCE_NEW err;
    irc->channels[irc->num_channels - 1].group_num = tox_conference_new(tox, &err);
    if (irc->channels[irc->num_channels - 1].group_num == UINT32_MAX) {
        DEBUG("Tox", "Could not create groupchat. Error number: %d", err);
        return false;
    }

    tox_conference_invite(tox, fid, irc->channels[irc->num_channels - 1].group_num, NULL);

    return true;
}

static bool command_leave(Tox *tox, IRC *irc, int fid, char *arg){
    if (!arg) {
        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)"An argument is required.", sizeof("An argument is required.") - 1, NULL);
        return false;
    }

    int index = irc_get_channel_index(irc, arg);
    if (index == -1) {
        DEBUG("Tox", "Could not get irc channel index.");
        return false;
    }

    irc_leave_channel(irc, index);
    tox_conference_delete(tox, irc->channels[index].group_num, NULL);

    return true;
}

static bool command_list(Tox *tox, IRC *irc, int fid, char *UNUSED(arg)){
    for (int i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].in_channel){
            tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (const unsigned char *)irc->channels[i].name, strlen(irc->channels[i].name), NULL);
        }
    }

    return true;
}

static bool command_id(Tox *tox, IRC *UNUSED(irc), int fid, char *UNUSED(arg)){
    uint8_t public_key_bin[TOX_ADDRESS_SIZE];
    char public_key_str[TOX_ADDRESS_SIZE * 2];

    tox_self_get_address(tox, public_key_bin);
    to_hex(public_key_str, public_key_bin, TOX_ADDRESS_SIZE);

    tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *)public_key_str, TOX_ADDRESS_SIZE * 2, NULL);

    return true;
}

static bool command_help(Tox *tox, IRC *UNUSED(irc), int fid, char *UNUSED(arg)){
    for (int i = 0; commands[i].cmd; i++){
        char *message = malloc(strlen(commands[i].cmd) + strlen(commands[i].desc) + 3);
        if (!message) {
            return false;
        }

        sprintf(message, "%s: %s", commands[i].cmd, commands[i].desc);

        tox_friend_send_message(tox, fid, TOX_MESSAGE_TYPE_NORMAL, (uint8_t *) message, strlen(message), NULL);
    }

    return true;
}
