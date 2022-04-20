#pragma once

#define FORMAT_ERROR 1
#define MALLOC_ERROR 2
#define FIFO_ERROR   3
#define OPEN_ERROR   4
#define WRITE_ERROR  5
#define READ_ERROR   6
#define FORK_ERROR   7
#define PIPE_ERROR   8

#include <stdlib.h>

typedef struct config
{
    int nop,
        bcompress,
        bdecompress,
        gcompress,
        gdecompress,
        encrypt,
        decrypt;

} Configuration;

void *xmalloc(size_t size);
int get_commands_len(char *string);
Configuration generate_config(char *path);