#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include "bot.h"
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

    if (!bot_init()) {
        return 1;
    }

    IRC *irc = irc_init(settings.server, settings.port);
    if (!irc) {
        return 2;
    }

    if (!irc_connect(irc, settings.name, settings.password)) {
        irc_free(irc);
        return 3;
    }

    irc_callbacks_setup(irc);

    bot_add_irc_server(irc);

    uint32_t num_groups = tox_conference_get_chatlist_size(bot.tox);
    if (num_groups > 0) {
        uint32_t groups[num_groups];
        tox_conference_get_chatlist(bot.tox, groups);

        for (uint32_t i = 0; i < num_groups; i++) {
            uint32_t title_size = tox_conference_get_title_size(bot.tox, groups[i], NULL);
            if (title_size == 0) {
                continue;
            }

            uint8_t title[title_size];
            tox_conference_get_title(bot.tox, groups[i], title, NULL);
            title[title_size] = '\0';

            irc_join_channel(irc, (char *)title, groups[i]);
        }
    } else {
        TOX_ERR_CONFERENCE_NEW err;
        uint32_t               group_num = tox_conference_new(bot.tox, &err);
        if (group_num == UINT32_MAX) {
            DEBUG("main", "Could not create groupchat for default group. Error number: %u", err);
            irc_disconnect(irc);
            irc_free(irc);
            return 4;
        }

        tox_conference_set_title(bot.tox, group_num, (const uint8_t *)settings.default_channel,
                                 strlen(settings.default_channel), NULL);
        irc_join_channel(irc, settings.default_channel, group_num);
    }

    while (!exit_bot) {
        irc_loop(irc, bot.tox);
        tox_iterate(bot.tox, irc);
        usleep(tox_iteration_interval(bot.tox));
    }

    bot_kill();

    settings_save(SETTINGS_FILE);

    return 0;
}
