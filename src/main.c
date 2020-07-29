#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include "irc.h"
#include "logging.h"
#include "macros.h"
#include "network.h"
#include "save.h"
#include "settings.h"
#include "tox.h"

#include "callbacks/irc_callbacks.h"

bool exit_bot = false;

static void signal_catch(int UNUSED(sig)) {
    exit_bot = true;
    printf("signal caught\n");
}

int main(void) {
    signal(SIGINT, signal_catch);

    if (!settings_load(SETTINGS_FILE)) {
        DEBUG("WARNING", "Settings could not be loaded, default settings will be used.");
    }

    DEBUG("main", "Starting bot");

    Tox *tox = tox_init();
    if (!tox) {
        return 1;
    }

    IRC *irc = irc_init(settings.server, settings.port);
    if (!irc) {
        tox_kill(tox);
        return 2;
    }

    if (!irc_connect(irc, settings.name, settings.password)) {
        irc_free(irc);
        tox_kill(tox);
        return 3;
    }

    irc_callbacks_setup(irc);

    uint32_t num_groups = tox_conference_get_chatlist_size(tox);
    if (num_groups > 0) {
        uint32_t groups[num_groups];
        tox_conference_get_chatlist(tox, groups);

        for (uint32_t i = 0; i < num_groups; i++) {
            uint32_t title_size = tox_conference_get_title_size(tox, groups[i], NULL);
            if (title_size == 0) {
                continue;
            }

            uint8_t title[title_size];
            tox_conference_get_title(tox, groups[i], title, NULL);
            title[title_size] = '\0';

            irc_join_channel(irc, (char *)title, groups[i]);
        }
    } else {
        TOX_ERR_CONFERENCE_NEW err;
        uint32_t               group_num = tox_conference_new(tox, &err);
        if (group_num == UINT32_MAX) {
            DEBUG("main", "Could not create groupchat for default group. Error number: %u", err);
            tox_kill(tox);
            irc_disconnect(irc);
            irc_free(irc);
            return 4;
        }

        tox_conference_set_title(tox, group_num, (const uint8_t *)settings.default_channel,
                                 strlen(settings.default_channel), NULL);
        irc_join_channel(irc, settings.default_channel, group_num);
    }

    while (!exit_bot) {
        irc_loop(irc, tox);
        tox_iterate(tox, irc);
        usleep(tox_iteration_interval(tox));
    }

    irc_disconnect(irc);
    irc_free(irc);

    save_write(tox, SAVE_FILE);
    tox_kill(tox);

    settings_save(SETTINGS_FILE);

    DEBUG("main", "Shutting down bot...");

    return 0;
}
