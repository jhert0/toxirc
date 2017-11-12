#include "irc.h"

#include "logging.h"
#include "settings.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

IRC *irc_connect(char *server, int port){
    IRC *irc = malloc(sizeof(IRC));
    if (!irc) {
        DEBUG("IRC", "Could not allocate memory for irc structure.");
        return NULL;
    }

    DEBUG("IRC", "Connecting to %s", server);

    irc->server = server;
    irc->port = port;

    irc->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (irc->sock < 0) {
        DEBUG("IRC", "Error creating socket");
        return NULL;
    }

    struct sockaddr_in addr_in;
    addr_in.sin_addr.s_addr = inet_addr(server);
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);

    if (connect(irc->sock , (struct sockaddr *)&addr_in , sizeof(addr_in)) < 0) {
        DEBUG("IRC", "Could not connect to %s", server);
        return NULL;
    }

    irc_send(irc->sock, "PASS none\n", sizeof("PASS none\n") - 1);
    irc_send_fmt(irc->sock, "NICK %s\n", settings.name);
    irc_send_fmt(irc->sock, "USER %s %s %s :%s\n", settings.name, settings.name, settings.name, settings.name);

    irc->connected = true;
    irc->channels = NULL;
    irc->num_channels = 0;
    irc->size_channels = 0;

    DEBUG("IRC", "Connected to %s", server);

    return irc;
}

bool irc_join_channel(IRC *irc, char *channel){
    if ((irc->num_channels + 1) >= irc->size_channels) {
        DEBUG("IRC", "Reallocating from %d to %d", irc->size_channels, irc->size_channels + 1);
        void *temp = realloc(irc->channels, irc->size_channels + 1);
        if (!temp) {
            DEBUG("IRC", "Could not reallocate memory from %d to %d.", irc->size_channels, irc->size_channels + 1);
            return false;
        }

        irc->channels = temp;

        irc->size_channels++;
    }

    irc_send_fmt(irc->sock, "JOIN %s\n", channel);

    irc->num_channels++;

    int index = irc->num_channels - 1;
    irc->channels[index].name = channel;
    irc->channels[index].in_channel = true;

    DEBUG("IRC", "Joining channel: %s", channel);

    return true;
}

bool irc_leave_channel(IRC *irc, int index){
    irc->num_channels--;

    irc_send_fmt(irc->sock, "PART %s\n", irc->channels[index].name);

    irc->channels[index].in_channel = false;

    return true;
}

void irc_disconnect(IRC *irc){
    irc_send(irc->sock, "QUIT\n", sizeof("QUIT\n") - 1);
    irc->connected = false;
    close(irc->sock);
    DEBUG("IRC", "Disconnected from server: %s.", irc->server);
}

void irc_leave_all_channels(IRC *irc){
    for (int i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].in_channel) {
            irc_leave_channel(irc, i);
        }
    }
}

void irc_free(IRC *irc){
    if (!irc) {
        return;
    }

    if (irc->channels) {
        free(irc->channels);
    }

    if (irc->connected) {
        irc_disconnect(irc);
    }

    free(irc);

    irc = NULL;
}

int irc_send(int sock, char *msg, int len){
    if (sock < 0) {
        DEBUG("IRC", "Bad socket. Unable to send data.");
        return -1;
    }

    int sent = 0, bytes = 0;

    while (sent < len) {
        bytes = send(sock, msg + sent, len - sent, MSG_NOSIGNAL);
        if (bytes <= 0) {
            DEBUG("IRC", "Problem sending data.");
            return -1;
        }

        sent += bytes;
    }

    return sent;
}

int irc_send_fmt(int sock, char *fmt, ...){
    char buf[512];
    va_list list;
    int len, sent;

    va_start(list, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);

    if (len > 512) {
        len = 512;
    }

    sent = irc_send(sock, buf, len);
    if (sent <= 0) {
        return -1;
    }

    return sent;
}

int irc_message(int sock, char *channel, char *name, char *msg){
    return irc_send_fmt(sock, "PRIVMSG %s :<%s> %s\r\n", channel, name, msg);
}

int irc_get_channel_index(IRC *irc, char *channel){
    for (int i = 0; i < irc->num_channels; i++) {
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

uint32_t irc_get_channel_group(IRC *irc, char *channel){
    for (int i = 0; i < irc->num_channels; i++){
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return UINT32_MAX;
}

char *irc_get_channel_by_group(IRC *irc, uint32_t group_num){
    for (int i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].group_num == group_num) {
            return irc->channels[i].name;
        }
    }

    return NULL;
}
