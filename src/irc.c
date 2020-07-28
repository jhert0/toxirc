#include "irc.h"

#include "logging.h"
#include "network.h"
#include "settings.h"

#include "commands/commands.h"
#include "commands/irc_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>

IRC *irc_init(char *server, char *port) {
    IRC *irc = malloc(sizeof(IRC));
    if (!irc) {
        DEBUG("IRC", "Could not allocate memory for irc structure.");
        return NULL;
    }

    memset(irc, 0, sizeof(IRC));

    irc->server = server;
    irc->port   = port;

    return irc;
}

bool irc_connect(IRC *irc, char *username, char *password) {
    DEBUG("IRC", "Connecting to %s:%s", irc->server, irc->port);

    struct addrinfo  hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_protocol = 0;

    int ret = getaddrinfo(irc->server, irc->port, &hints, &result);
    if (ret != 0) {
        DEBUG("IRC", "Error getting address information for %s.", irc->server);
        return false;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        irc->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (irc->sock == -1) {
            continue;
        }

        if (connect(irc->sock, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        close(irc->sock);
    }

    freeaddrinfo(result);

    network_send_fmt(irc->sock, "PASS %s\n", password);
    network_send_fmt(irc->sock, "NICK %s\n", username);
    network_send_fmt(irc->sock, "USER %s %s %s :%s\n", username, username, username, username);

    irc->connected = true;
    if (!irc->nick) {
        irc->nick = malloc(strlen(username));
        if (!irc->nick) {
            return false;
        }
        memcpy(irc->nick, username, strlen(username));
    }

    DEBUG("IRC", "Connected to %s", irc->server);

    return true;
}

bool irc_reconnect(IRC *irc) {
    if (irc->connected) {
        irc_disconnect(irc);
    }

    if (!irc_connect(irc, irc->nick, settings.password)) {
        return false;
    }

    for (uint32_t i = 0; i < irc->num_channels; i++) {
        irc_rejoin_channel(irc, i);
    }

    return true;
}

bool irc_join_channel(IRC *irc, char *channel, uint32_t group_num) {
    if (group_num >= irc->size_channels) {
        DEBUG("IRC", "Reallocating from %d to %d", irc->size_channels, group_num + 1);
        void *temp = realloc(irc->channels, sizeof(Channel) * (group_num + 1));
        if (!temp) {
            DEBUG("IRC", "Could not reallocate memory from %d to %d.", irc->size_channels, group_num);
            return false;
        }

        irc->channels = temp;

        irc->size_channels++;
        irc->num_channels++;

        memset(&irc->channels[irc->num_channels - 1], 0, sizeof(Channel));
    }

    network_send_fmt(irc->sock, "JOIN %s\n", channel);

    size_t length = strlen(channel);
    if (length > IRC_MAX_CHANNEL_LENGTH) {
        length = IRC_MAX_CHANNEL_LENGTH;
    }

    const uint32_t index = irc->num_channels - 1;
    memcpy(irc->channels[index].name, channel, length);
    irc->channels[index].in_channel  = true;
    irc->channels[index].group_num   = group_num;
    irc->channels[index].name_length = length;

    DEBUG("IRC", "Joining channel: %s", channel);

    return true;
}

void irc_rejoin_channel(IRC *irc, uint32_t index) {
    irc->channels[index].in_channel = true;
    network_send_fmt(irc->sock, "JOIN %s\n", irc->channels[index].name);
}

bool irc_leave_channel(IRC *irc, uint32_t index) {
    network_send_fmt(irc->sock, "PART %s\n", irc->channels[index].name);

    memset(&irc->channels[index], 0, sizeof(Channel));

    irc->num_channels--;

    return true;
}

void irc_disconnect(IRC *irc) {
    network_send(irc->sock, "QUIT\n", sizeof("QUIT\n") - 1);
    irc->connected = false;
    close(irc->sock);
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        irc->channels[i].in_channel = false;
    }
    DEBUG("IRC", "Disconnected from server: %s.", irc->server);
}

void irc_leave_all_channels(IRC *irc) {
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].in_channel) {
            irc_leave_channel(irc, i);
        }
    }
}

void irc_free(IRC *irc) {
    if (!irc) {
        return;
    }

    if (irc->connected) {
        irc_disconnect(irc);
    }

    if (irc->channels) {
        free(irc->channels);
    }

    if (irc->nick) {
        free(irc->nick);
    }

    free(irc);

    irc = NULL;
}

int irc_message(IRC *irc, char *channel, char *msg) {
    return network_send_fmt(irc->sock, "PRIVMSG %s :%s\n", channel, msg);
}

uint32_t irc_get_channel_index(const IRC *irc, const char *channel) {
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return UINT32_MAX;
}

uint32_t irc_get_channel_group(const IRC *irc, const char *channel) {
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return UINT32_MAX;
}

char *irc_get_channel_by_group(const IRC *irc, uint32_t group_num) {
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].group_num == group_num) {
            return irc->channels[i].name;
        }
    }

    return NULL;
}

bool irc_in_channel(const IRC *irc, const char *channel) {
    for (uint32_t i = 0; i < irc->num_channels; i++) {
        if (strcmp(irc->channels[i].name, channel) == 0 && irc->channels[i].in_channel) {
            return true;
        }
    }

    return false;
}

void irc_loop(IRC *irc, void *userdata) {
    int count = 0;

    ioctl(irc->sock, FIONREAD, &count);

    if (count > 0) {
        uint8_t data[count + 1];
        data[count] = 0;
        recv(irc->sock, data, count, MSG_NOSIGNAL);
        printf("%s", data);

        if (strncmp((char *)data, "PING", 4) == 0) {
            data[1] = 'O';

            int i;
            for (i = 0; i < count; ++i) {
                if (data[i] == '\n') {
                    ++i;
                    break;
                }
            }

            network_send(irc->sock, (char *)data, i);
        } else if (strncmp((char *)data, "ERROR", 4) == 0) {
            DEBUG("main", "Disconnected from the irc server reconnecting...");
            if (!irc_reconnect(irc)) {
                DEBUG("main", "Unable to reconnect. Dying...");
                return;
            }
        } else if (data[0] == ':') {
            irc->message_callback(irc, (char *)data, userdata);
        }

        int       error = 0;
        socklen_t len   = sizeof(error);
        if (irc->sock < 0 || getsockopt(irc->sock, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
            DEBUG("main", "Socket has gone bad. Error: %d. Reconnecting...", error);
            if (irc_reconnect(irc)) {
                DEBUG("main", "Unable to reconnect. Dying...");
                return;
            }
        }
    }
}

void irc_set_message_callback(IRC *irc, void (*func)(IRC *irc, char *message, void *userdata)) {
    irc->message_callback = func;
}
