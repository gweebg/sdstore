/**
 * @file server.c
 * @author gweebg ; johnny_longo 
 * @brief Server side of the application. 
 * @version 0.1
 * @date 2022-04-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

// #include "../includes/server.h"

int main()
{
    /* cts <=> client_to_server */
    int client_to_server;
    const char *cts_fifo = "com/cts";
    mkfifo(cts_fifo, 0666);

    /* stc <=> server_to_client */
    int server_to_client;
    const char *stc_fifo = "com/stc";
    mkfifo(stc_fifo, 0666);

    client_to_server = open(cts_fifo, O_RDONLY);
    server_to_client = open(stc_fifo, O_WRONLY);

    write(STDOUT_FILENO, "[!] Server is online!\n" , 23);
    write(STDOUT_FILENO, "[*] Listening for data...\n", 27);

    char buffer[1024];
    while(true)
    {
        if (read(client_to_server, buffer, 1024) == -1)
        {
            fprintf(stderr, "[!] Could not read from FIFO. Aborting...\n");
            return 1;
        }

        if (strcmp("exit", buffer) == 0)
        {
            printf("[!] Server is offline!\n");
            break;
        }

        write(STDOUT_FILENO, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer));
    }

    close(client_to_server);
    close(server_to_client);

    // unlink(cts_fifo);
    // unlink(stc_fifo);

    return 0;
}