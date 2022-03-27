#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bot.h"
#include "tox.h"
#include "save.h"
#include "irc.h"
#include "logging.h"

Bot *bot_init() {
    DEBUG("Bot", "Starting bot...");

    Bot *bot = malloc(sizeof(Bot));
    if (!bot) {
        DEBUG("Bot", "Could not allocate memory for bot structure.");
        return NULL;
    }

    memset(bot, 0, sizeof(Bot));

    bot->tox = tox_init();
    if (!bot->tox) {
        DEBUG("Bot", "Could not create tox instance.");
        return NULL;
    }

    return bot;
}

bool bot_add_irc_server(Bot *bot, IRC *irc) {
    if ((bot->num_servers + 1) >= bot->server_size) {
        DEBUG("Bot", "Reallocating from %d to %d", bot->server_size, bot->num_servers + 1);
        void *temp = realloc(bot->irc, sizeof(IRC) * (bot->num_servers + 1));
        if (!temp) {
            DEBUG("Bot", "Could not reallocate from %d to %d.", bot->server_size, bot->num_servers + 1);
            return false;
        }

        bot->irc = temp;

        bot->num_servers++;
        bot->server_size++;
    } else {
        bot->num_servers++;
    }

    bot->irc[bot->num_servers - 1] = irc;

    return true;
}

void bot_remove_irc_server(Bot *bot, uint32_t index) {
    DEBUG("Bot", "Removing server %d from the bot.", index);

    IRC *irc = bot->irc[index];
    irc_disconnect(irc);
    irc_free(irc);

    bot->num_servers--;
}

void bot_free(Bot *bot) {
    if (!bot) {
        return;
    }

    if (bot->irc) {
        for (uint32_t i = 0; i < bot->num_servers; i++) {
            irc_free(bot->irc[i]);
        }

        free(bot->irc);
    }

    save_write(bot->tox, SAVE_FILE);
    tox_kill(bot->tox);

    free(bot);

    bot = NULL;
}
