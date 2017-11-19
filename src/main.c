#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
    struct sigaction act;
    act.sa_handler = signal_catch;
    sigaction(SIGINT, &act, NULL);

    if (!settings_load(SETTINGS_FILE)) {
        DEBUG("WARNING", "Settings could not be loaded, default settings will be used.");
    }

    DEBUG("main", "Starting bot");

    Tox *tox = tox_init();
    if (!tox) {
        return 1;
    }

    IRC *irc = irc_connect(settings.server, settings.port);
    if (!irc) {
        tox_kill(tox);
        return 2;
    }

    TOX_ERR_CONFERENCE_NEW err;
    uint32_t group_num = tox_conference_new(tox, &err);
    if (group_num == UINT32_MAX){
        DEBUG("main", "Could not create groupchat for default group.");
        tox_kill(tox);
        irc_disconnect(irc);
        irc_free(irc);
        return 3;
    }

    irc_join_channel(irc, settings.default_channel, group_num);

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
                char nick[32], user[32], server[32], channel[50], msg[256];
                int matches = sscanf((char *)data, ":%31[^!]!~%31[^@]@%31s PRIVMSG %49s :%255[^\r\n]", nick, user, server, channel, msg);
                if (matches != 5) {
                    continue;
                }

                uint32_t group = irc_get_channel_group(irc, channel);
                if (group == UINT32_MAX) {
                    continue;
                }

                tox_group_send_msg(tox, group, nick, msg);
            }
        }

        int error = 0;
        socklen_t len = sizeof(error);

        if (irc->sock < 0 || getsockopt(irc->sock, SOL_SOCKET, SO_ERROR, &error, &len) != 0) {
            DEBUG("main", "Socket has gone bad. Reconnecting...");
            irc_disconnect(irc);
            irc_free(irc);
            irc = irc_connect(settings.server, settings.port);
        }

        tox_iterate(tox, irc);
        usleep(tox_iteration_interval(tox));
    }

    irc_disconnect(irc);
    irc_free(irc);

    write_config(tox, SAVE_FILE);
    tox_kill(tox);

    settings_save(SETTINGS_FILE);

    DEBUG("main", "Shutting down bot...");

    return 0;
}
