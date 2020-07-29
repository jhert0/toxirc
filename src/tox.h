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
 * returns true on success
 * returns false on failure
 */
bool tox_connect(Tox *tox);

/*
 * Send a message to a tox groupchat formated as "<nick> msg"
 */
void tox_group_send_msg(Tox *tox, uint32_t group_num, char *nick, char *msg);

/*
 * Checks if the specified friend is the bot's master
 */
bool tox_is_friend_master(Tox *tox, uint32_t fid);

#endif
