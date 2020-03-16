#include "network.h"

#include "logging.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>

int network_send(int sock, char *msg, int len) {
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

int network_send_fmt(int sock, char *fmt, ...) {
    char    buf[512];
    va_list list;
    int     len, sent;

    va_start(list, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);

    if (len > 512) {
        len = 512;
    }

    sent = network_send(sock, buf, len);
    if (sent <= 0) {
        return -1;
    }

    return sent;
}
