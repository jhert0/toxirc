#ifndef _TOX_H
#define _TOX_H

#include <tox/tox.h>

#define NAME "syncbot"
#define STATUS "Send me h for more info"

/*
 * Initiliazes tox
 * returns a Tox struct on success
 * returns NULL on failure
 */
Tox *tox_init();

bool tox_connect(Tox *tox);

void tox_send_group_msg(Tox *tox, uint32_t group_num, char *nick, size_t nick_lengh, char *msg, size_t msg_length);

/*
 * Creates a tox groupchat
 * returns group number on success
 */
int tox_create_groupchat(Tox *tox);

#endif
