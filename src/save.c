#include "save.h"

#include "logging.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tox/tox.h>

bool write_config(Tox *tox, char *path){
    FILE *fp = fopen(path, "wb+");
    if (!fp) {
        DEBUG("Save", "Can not open: %s", path);
        return false;
    }

    size_t size = tox_get_savedata_size(tox);
    uint8_t *data = malloc(size + 1);
    if (!data) {
        DEBUG("Save", "Could not allocate memory for save data.");
        return false;
    }

    tox_get_savedata(tox, data);

    if (fwrite(data, size, 1, fp) != 1) {
        DEBUG("Save", "Could not write save data to %s", path);
        free(data);
        fclose(fp);
        return false;
    }

    free(data);
    fclose(fp);

    return true;
}

Tox *load_config(char *path, int *status){
    Tox *tox = NULL;
    struct Tox_Options options;

    memset(&options, 0, sizeof(struct Tox_Options));
    tox_options_default(&options);

    //TODO: make these an argument or option
    tox_options_set_udp_enabled(&options, false);
    tox_options_set_ipv6_enabled(&options, false);

    int fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); //TODO: this needs to be improved
    FILE *fp = fdopen(fd, "rb");
    if (!fp) {
        DEBUG("Save", "Could not open %s", path);
        return NULL;
    }

    off_t size = get_file_size(fp);
    if (size == 0) {
        DEBUG("Save", "Could not get the file size for %s. Assuming new profile.", path);
        fclose(fp);
        tox = tox_new(&options, NULL);
        *status = 2;
        return tox;
    }

    uint8_t data[size];

    if (fread(data, size, 1, fp) != 1) {
        DEBUG("Save", "Could not read the save data from %s", path);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    options.savedata_data = data;
    options.savedata_length = size;
    options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;

    TOX_ERR_NEW err;
    tox = tox_new(&options, &err);

    if (err != TOX_ERR_NEW_OK) {
        DEBUG("Tox", "Failed to create tox instance. Error number: %d", err);
        return NULL;
    }

    *status = 1;

    return tox;
}
