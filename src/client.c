/**
 * @file client.c
 * @author gweebg ; johnny_longo ; Miguel-Neiva
 * @brief Client side of the application. 
 * @version 0.1
 * @date 2022-04-29
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "../includes/client.h"
#include "../includes/utils.h"

/**
 * @brief Funtion that executes the whole client side as a programm.
 * 
 * @param argc Number or arguments.
 * @param argv Array containing command line arguments.
 * @return Error code (int).
 */
int main(int argc, char *argv[]) 
{
    /*
    Exemplo de input:
    ./sdstore proc-file -p <priority> samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop
    */

    /* Abrir comunicacao com o servidor. */ 
    int client_to_server = open("tmp/cts", O_WRONLY);
    if (client_to_server < 0)
    {
        print_error("Failed to open client to server pipe (client).\n");
        return OPEN_ERROR;
    }

    /* Criacao do pipe entre servidor e cliente. */
    pid_t client_id = getpid();

    char cts_fifo[64];
    sprintf(cts_fifo, "tmp/stc_%d", client_id);

    char info[32];
    sprintf(info, "Job id: %d\n", client_id);
    print_info(info);

    /* Enviar sinal de "status" ou "help" ao servidor. */
    if ((argc >= 6) || (strcmp(argv[1], "status") == 0) || (strcmp(argv[1], "help") == 0)) /* Enviar os argumentos todos numa string para o servidor. */
    {
        mkfifo(cts_fifo, 0666);

        char *message = xmalloc(sizeof(char) * (argc) * 16);
        strcpy(message, cts_fifo);

        for (int i = 1; i < argc; i++)
        {
            char *temp = xmalloc(sizeof(char) * 16);        
            sprintf(temp, " %s", argv[i]);

            strncat(message, temp, strlen(argv[i]) + 1);
            free(temp);
        }

        if (write(client_to_server, message, strlen(message)) < 0)
        {
            print_error("Failed to write to client to server pipe.\n");
            return WRITE_ERROR;
        }
    }
    else /* Erro se houver menos de 6 argumentos ou argumentos invalidos. */
    {
        print_error("Not enought arguments. Expected at least 6 but got less, refer to the documentation for more information.\n");
        return FORMAT_ERROR;
    }

    close(client_to_server);

    int server_to_client = open(cts_fifo, O_RDONLY);
    if (server_to_client < 0)
    {
        print_error("Failed to open server to client pipe. (client).\n");
        return OPEN_ERROR;
    }

    int bytes_read; char string[BUFSIZ];

    /* Listen to incoming messages from the server. */
    while(true)
    {
        while((bytes_read = read(server_to_client, string, BUFSIZ)) > 0) 
        {   
            write(STDOUT_FILENO, string, bytes_read);

            /* 848 is the size of the help message. */
            if (bytes_read >= 848) return EXIT_SUCCESS;
            if (strncmp(string, "[*] Completed", 13) == 0) return EXIT_SUCCESS;
            if (strncmp(string, "[SERVER STATUS]", 15) == 0) return EXIT_SUCCESS;

        }
    }

    close(server_to_client);
    return 0;
}
