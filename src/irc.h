#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IRC_MAX_CHANNEL_LENGTH 50
#define IRC_MAX_NICK_LENGTH 50
#define IRC_PORT_LENGTH 5
#define IRC_MAX_PASSWORD_LENGTH 50
#define IRC_MAX_MESSGE_LENGTH 512

struct channel {
    char     name[IRC_MAX_CHANNEL_LENGTH];
    size_t   name_length;
    uint32_t group_num;
    bool     in_channel;
};

struct irc {
    int   sock;
    char *server;
    char *port;
    bool  connected;

    char *nick;

    // Channel data
    struct channel *channels;
    uint32_t        num_channels;
    uint32_t        size_channels;

    // Callbacks
    void (*message_callback)(struct irc *irc, char *message, void *userdata);
};

enum irc_message_type { IRC_MESSAGE_REPLY, IRC_MESSGAE_PRIVMSG };
typedef enum irc_message_type irc_message_type;

struct irc_message {
    irc_message_type type;

    int  code;
    char server[100];
    char channel[IRC_MAX_CHANNEL_LENGTH];
    char nick[IRC_MAX_NICK_LENGTH];
    char message[IRC_MAX_MESSGE_LENGTH];
};

typedef struct irc         IRC;
typedef struct channel     Channel;
typedef struct irc_message irc_message;

/*
 * Initializes the IRC structure
 * returns an IRC struct on succes
 * returns NULL on failure
 */
IRC *irc_init(char *server, char *port);

/*
 * Connects to the IRC server
 * returns true on success
 * returns false on failure
 */
bool irc_connect(IRC *irc, char *username, char *password);

/*
 * Reconnects to the IRC server
 * returns true on success
 * returns false on failure
 */
bool irc_reconnect(IRC *irc);

/*
 * Joins the specified IRC channel
 * returns true on success
 * returns false on failure
 */
bool irc_join_channel(IRC *irc, char *channel, uint32_t group_num);

/*
 * Rejoins the channel at the specified index
 */
void irc_rejoin_channel(IRC *irc, uint32_t index);

/*
 * Leaves the specified IRC channel
 * returns true on success
 * returns false on failure
 */
bool irc_leave_channel(IRC *irc, uint32_t index);

void irc_delete_channel(IRC *irc, uint32_t index);

/*
 * Disconnects from the IRC server
 */
void irc_disconnect(IRC *irc);

/*
 * Leaves all channels
 */
void irc_leave_all_channels(IRC *irc);

/*
 * Sends the specified message to the specified channel
 */
int irc_send_message(IRC *irc, char *channel, char *msg);

/*
 * Sends the specified action message to the specified channel
 */
int irc_send_action_message(IRC *irc, char *channel, char *msg);

/*
 * Frees the IRC struct and irc->channels.
 * If the connection hasn't been closed it will also disconnect from the IRC
 * server
 */
void irc_free(IRC *irc);

/*
 * Gets the specifed channel's index
 * returns the index on success
 * returns UINT32_MAX on failure
 */
uint32_t irc_get_channel_index(const IRC *irc, const char *channel);

/*
 * Gets the specified channel's group number
 * returns the group number on success
 * returns UINT32_MAX on failure
 */
uint32_t irc_get_channel_group(const IRC *irc, const char *channel);

/*
 * Gets the channel name for the specified group number
 * returns the channel name on success
 * returns NULL on failure
 */
char *irc_get_channel_by_group(const IRC *irc, uint32_t group_num);

/*
 * Checks if the bot is in the specified irc channel
 * returns true on success
 * returns false on failure
 */
bool irc_in_channel(const IRC *irc, const char *channel);

/*
 *
 */
void irc_loop(IRC *irc, void *userdata);

irc_message *irc_parse_message(char *buffer);

/*
 * Set the message callback that will be used in irc_loop
 */
void irc_set_message_callback(IRC *irc, void (*func)(IRC *irc, char *message, void *userdata));

#endif
