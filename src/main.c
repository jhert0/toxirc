#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stropts.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "irc.h"
#include "logging.h"
#include "macros.h"
#include "save.h"
#include "settings.h"
#include "tox.h"

bool exit_bot = false;

static void signal_catch(int UNUSED(sig)){
    exit_bot = true;
    printf("signal caught\n");
}

int main(void){
    DEBUG("main", "Starting bot...");

    struct sigaction act;
    act.sa_handler = signal_catch;
    sigaction(SIGINT, &act, NULL);

    Tox *tox = tox_init();
    if (!tox) {
        return 1;
    }


    IRC *irc = irc_connect("71.11.84.232", 6667);
    if (!irc) {
        return 2;
    }

    irc_join_channel(irc, "#toxirc");
    irc->channels[irc->num_channels - 1].group_num = tox_conference_new(tox, NULL);

    char nick[128], msg[512], channel[128];
    while (!exit_bot) {
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

                irc_send(irc->sock, (char *)data, i);
            } else if(data[0] == ':') {
                char *ptr = strtok((char *)data, "!");
                bool privmsg = false;

                if (ptr == NULL) {
                    continue;
                } else {
                    strncpy(nick, &ptr[1], 127);
                    nick[127] = '\0';
                }

                if (strcmp(nick, settings.name) == 0) {
                    continue;
                }

                while ((ptr = strtok(NULL, " ")) != NULL) {
                    if (strcmp(ptr, "PRIVMSG") == 0) {
                        privmsg = true;
                        break;
                    }
                }

                if (!privmsg) {
                    continue;
                }

                if ((ptr = strtok(NULL, ":")) != NULL && (ptr = strtok(NULL, "")) != NULL) {
                    strncpy(msg, ptr, 511);
                    msg[511] = '\0';
                    printf("message: %s\n", msg);
                }

                tox_group_send_msg(tox, 0, nick, msg); //TODO: use the channel name to get the group number
            }
        }

        int error = 0;
        socklen_t len = sizeof(error);

        if (irc->sock < 0 || getsockopt(irc->sock, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
            DEBUG("main", "Socket has gone bad. Reconnecting...");
            irc_disconnect(irc);
            irc_free(irc);
            irc = irc_connect("71.11.84.232", 667);
        }

        tox_iterate(tox, irc);
        usleep(tox_iteration_interval(tox));
    }

    irc_disconnect(irc);
    irc_free(irc);

    write_config(tox, SAVE_FILE);
    tox_kill(tox);

    DEBUG("main", "Shutting down bot...");

    return 0;
}
