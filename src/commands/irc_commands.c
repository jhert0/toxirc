#include "irc_commands.h"

#include "commands.h"

#include "../macros.h"
#include "../settings.h"

#include <stdio.h>
#include <string.h>
#include <tox/tox.h>

static bool command_users(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_topic(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_help(Tox *tox, IRC *irc, uint32_t index, char *arg);
static bool command_characters(Tox *tox, IRC *irc, uint32_t index, char *arg);

// clang-format off
// TODO: when new groupchats are merged add in a command for getting the groupchats id
struct Command irc_commands[MAX_CMDS] = {
    { "users",      " Retrieve the users in the tox groupchat",                            false, command_users      },
    { "topic",      "Gets the topic for the groupchat this channel is being synced with.", false, command_topic      },
    { "characters", "Gives information about all the special characters",                  false, command_characters },
    { "help",       "This message.",                                                       false, command_help       },
    { NULL,         NULL,                                                                  false, NULL               },
};
// clang-format on

static bool command_users(Tox *tox, IRC *irc, uint32_t index, char *UNUSED(arg)) {
    uint32_t group_number = irc->channels[index].group_num;

    uint32_t peer_count = tox_conference_peer_count(tox, group_number, NULL);
    uint8_t  names[peer_count][TOX_MAX_MESSAGE_LENGTH];

    size_t names_size = 0;
    size_t name_lens[peer_count];
    for (uint32_t i = 0; i < peer_count; i++) {
        Tox_Err_Group_Peer_Query err;
        name_lens[i] = tox_group_peer_get_name_size(tox, group_number, i, &err);
        if (name_lens[i] == 0 || err != TOX_ERR_GROUP_PEER_QUERY_OK) {
            memcpy(names[i], "unknown", 7);
            name_lens[i] = 7;
        } else {
            tox_group_peer_get_name(tox, group_number, i, names[i], NULL);
        }
        names[i][name_lens[i]] = '\0';
        names_size += name_lens[i];
    }

    char buffer[33 + names_size +
                peer_count]; // 33 for string, names_size for the names, peer_count for spaces in between names
    snprintf(buffer, sizeof(buffer), "There are %u users in the groupchat: ", peer_count);

    for (uint32_t i = 0; i < peer_count; i++) {
        strncat(buffer, (const char *)names[i], name_lens[i]);
        strcat(buffer, " ");
    }

    irc_send_message(irc, irc->channels[index].name, buffer);

    return true;
}

static bool command_topic(Tox *tox, IRC *irc, uint32_t index, char *UNUSED(arg)) {
    uint32_t group_num  = irc->channels[index].group_num;
    size_t   topic_size = tox_group_get_topic_size(tox, group_num, NULL);

    uint8_t topic[topic_size];
    tox_group_get_topic(tox, group_num, topic, NULL);
    topic[topic_size] = '\0';
    topic_size++;

    char buffer[14 + topic_size];
    sprintf(buffer, "The topic is: %s", topic);

    irc_send_message(irc, irc->channels[index].name, buffer);

    return true;
}

static bool command_help(Tox *UNUSED(tox), IRC *irc, uint32_t index, char *UNUSED(arg)) {
    char *cmd_prefix = settings_get_prefix(CHAR_CMD_PREFIX);
    for (int i = 0; irc_commands[i].cmd; i++) {
        char message[strlen(cmd_prefix) + strlen(irc_commands[i].cmd) + strlen(irc_commands[i].desc) + 3];
        sprintf(message, "%s%s: %s", cmd_prefix, irc_commands[i].cmd, irc_commands[i].desc);
        irc_send_message(irc, irc->channels[index].name, message);
    }

    return true;
}

static bool command_characters(Tox *UNUSED(tox), IRC *irc, uint32_t index, char *UNUSED(arg)) {
    for (unsigned int i = 0; i < CHAR_MAX; i++) {
        char message[strlen(settings.characters[i].prefix) + strlen(settings.characters[i].desc) + 2];
        sprintf(message, "%s: %s", settings.characters[i].prefix, settings.characters[i].desc);
        irc_send_message(irc, irc->channels[index].name, message);
    }

    return true;
}
