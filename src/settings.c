#include "settings.h"

#include <stdbool.h>

enum SECTION {
    SECTION_BOT,
    SECTIO_TOX,
    SECTION_IRC,
    SECTION_UNKNOWN,
};

static const char *sections[SECTION_UNKNOWN + 1] = {
    "Bot",
    "Tox",
    "IRC",
    NULL,
};

// default settings
SETTINGS settings = {
    .name = "toxirc",
    .status = "Send me help for more info.",
    .ipv6 = true,
    .udp = false,
    .master = "",
    .default_channel = "#toxirc",
};

void settings_write_string(){

}

void settings_save(char *file){

}

void settings_load(char *file){

}
