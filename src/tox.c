#include "tox.h"

#include "logging.h"
#include "nodes.h"
#include "save.h"
#include "settings.h"
#include "utils.h"

#include "callbacks/tox_callbacks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tox/tox.h>

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

    tox_callbacks_setup(tox);

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
