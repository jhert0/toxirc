#ifndef COMMANDS_H
#define COMMANDS_H

struct command {
    char *cmd;
    void (*func)(void *object, char *arg, int arg_length);
};

typedef struct command COMMAND;

#endif
