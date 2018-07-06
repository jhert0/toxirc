#ifndef _TOX_H
#define _TOX_H

#include <tox/tox.h>

/*
 * Initiliazes tox
 * returns a Tox struct on success
 * returns NULL on failure
 */
Tox *tox_init();

/*
 * Connects you to the Tox network
 * on success returns true
 * on failure returns false
 */
bool tox_connect(Tox *tox);

void tox_group_send_msg(Tox *tox, uint32_t group_num, char *nick, char *msg);

bool tox_is_friend_master(Tox *tox, uint32_t fid);

#endif
