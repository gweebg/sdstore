#pragma once

#define FORMAT_ERROR 1
#define MALLOC_ERROR 2
#define FIFO_ERROR   3
#define OPEN_ERROR   4
#define WRITE_ERROR  5
#define READ_ERROR   6
#define FORK_ERROR   7
#define PIPE_ERROR   8

#define QSIZE        1024

#include <stdlib.h>

/**
 * @brief Struct that stores the number of times an operation can run at the same time.
 * 
 * @param nop
 * @param bcompress
 * @param bdecompress
 * @param gcompress
 * @param gdecompress
 * @param encrypt
 * @param decrypt
 */
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

Configuration generate_config(char *path);

int get_commands_len(char *string);

void *xmalloc(size_t size);

void print_error(char *content);

void print_info(char *content);

void print_log(char *content);