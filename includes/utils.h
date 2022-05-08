#include <stdlib.h>
#include <stdbool.h>

#include "llist.h"

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

void print(char *content);

void print_log(char *content, int log_file, bool print_to_terminal);

void send_help_message(int server_to_client);

void print_server_help();

void generate_status_message_from_queued(char *dest, struct Node *llist, char *fifo_id);

void generate_status_message_from_executing(char *dest, struct Node *llist);

void generate_completed_message(char *dest, char *in, char *out);

void send_status_to_client(char *fifo, char *content);