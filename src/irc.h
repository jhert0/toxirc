#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IRC_MAX_CHANNEL_LENGTH 50
#define IRC_PORT_LENGTH 5

struct channel {
    char name[IRC_MAX_CHANNEL_LENGTH];
    size_t name_length;
    uint32_t group_num;
    bool in_channel;
};

struct irc {
    int sock;
    char *server;
    char *port;
    bool connected;

    //Channel data
    struct channel *channels;
    uint32_t num_channels;
    uint32_t size_channels;

    //Callbacks
    void (*join_callback)(struct irc *irc, char *channel, void *userdata);
    void (*leave_callback)(struct irc *irc, char *channel, void *userdata);
    void (*list_callback)(struct irc *irc, char *channel, void *userdata);
    void (*message_callback)(struct irc *irc, char *message, void *userdata);
};

typedef struct irc IRC;
typedef struct channel Channel;

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
bool irc_connect(IRC *irc);

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
int irc_message(IRC *irc, char *channel, char *msg);

/*
 * Frees the IRC struct and irc->channels.
 * If the connection hasn't been closed it will also disconnect from the IRC server
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
 * on success returns the channel name
 * on failure returns NULL
 */
char *irc_get_channel_by_group(const IRC *irc, uint32_t group_num);

/*
 * Checks if the bot is in the specified irc channel
 * on success returns true
 * on failure returns false
 */
bool irc_in_channel(const IRC *irc, const char *channel);

int irc_command_list(IRC *irc, char *channel, char *users);

int irc_command_topic(IRC *irc, char *channel);

void irc_loop(IRC *irc, void *userdata);

void irc_set_message_callback(IRC *irc, void (*func)(IRC *irc, char *message, void *userdata));

void irc_set_join_callback(IRC *irc, void (*func)(IRC *irc, char *channel, void *userdata));

void irc_set_leave_callback(IRC *irc, void (*func)(IRC *irc, char *channel, void *userdata));

void irc_set_list_callback(IRC *irc, void(*func)(IRC *irc, char *channel, void *userdata));

#endif
