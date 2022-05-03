#include <stdlib.h>
#include <stdbool.h>

#pragma once

#define FORMAT_ERROR 1
#define MALLOC_ERROR 2
#define FIFO_ERROR   3
#define OPEN_ERROR   4
#define WRITE_ERROR  5
#define READ_ERROR   6
#define FORK_ERROR   7
#define PIPE_ERROR   8
#define DUP2_ERROR   9
#define EXEC_ERROR   10

#define QSIZE        1024

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

int total_operations(char *string);

void *xmalloc(size_t size);

void print_error(char *content);

void print_info(char *content);

void print_log(char *content, int log_file, bool print_to_terminal);

void send_help_message(int server_to_client);