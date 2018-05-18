#ifndef NETWORK_H
#define NETWORK_H

/*
 * Send msg with length len to the IRC server
 * on success returns the number of bytes sent
 * on failure returns -1
 */
int network_send(int sock, char *msg, int len);

/*
 * Send a formated message to the IRC server
 * on success returns the number of bytes sent
 * on failure returns -1
 */
int network_send_fmt(int sock, char *fmt, ...);

#endif
