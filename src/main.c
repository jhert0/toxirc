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
#include "tox.h"

bool exit_bot = false;

static void signal_catch(int UNUSED(sig)){
    exit_bot = true;
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


    irc_join_channel(irc, "#syncbot_test");

    char buf[512], nick[128], msg[512], channel[128];
    while (!exit_bot) {
        recv(irc->sock, buf, sizeof(buf) - 1, MSG_NOSIGNAL);
        if (strncmp(buf, "PING :", 6) == 0) {
            DEBUG("main", "Ping received");
            buf[1] = 'O';
            irc_send(irc->sock, buf, 6);
        } else if (strncmp(buf, "NOTICE AUTH :", 13) == 0) {
            continue;
        } else if (strncmp(buf, "ERROR :", 7) == 0) {
            DEBUG("main", "Received an error.");
            //irc->connected = false;
            //break;
            continue;
        } else if (buf[0] == ':') { //TODO: parse channel
            char *ptr = strtok(buf, "!");
            bool privmsg = false;

            if (ptr == NULL) {
                continue;
            } else {
                strncpy(nick, &ptr[1], 127);
                nick[127] = '\0';
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
                printf("%s\n", msg);
            }

            tox_send_group_msg(tox, 0, nick, strlen(nick), msg, strlen(msg)); //TODO: use the channel name to get the group number
        }

        tox_iterate(tox, (void *)irc);
        usleep(1000 * 50);
        bzero(buf, sizeof(buf));
    }

    irc_disconnect(irc);
    irc_free(irc);

    write_config(tox, SAVE_FILE);
    tox_kill(tox);

    DEBUG("main", "Shutting down bot...");

    return 0;
}
