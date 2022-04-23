/**
 * @file client.c
 * @author gweebg ; johnny_longo 
 * @brief Client side of the application. 
 * @version 0.1
 * @date 2022-04-12
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

#define DEBUG

/**
 * @brief Function that halts the execution because of an write error.
 */
void raise_write_error()
{
    write(STDERR_FILENO, "[!] Could not write to FIFO.\n", 30);
    exit(WRITE_ERROR);
}

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
    ./sdstore proc-file <priority> samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop
    Menos de 6 argumentos raise erro.
    Also, o enunciado diz para assumir que o input é sempre válido.
    */

    if (argc < 6 )
    {
        if (!(argc == 2 && ((strcmp(argv[1], "status") == 0) || (strcmp(argv[1], "help") == 0))))
        {
            print_error("Not enought arguments. Expected at least 6 but got less, refer to the documentation for more information.\n");
            return FORMAT_ERROR;
        }
    }

    char *arguments = xmalloc(sizeof(char) * 32 * (argc - 1));

    /* Meter todos os argumentos numa string para poder enviar mais facilmente para o servidor. */
    for (int i = 1; i < argc; i++)
    {
        char *temp = xmalloc(sizeof(char) * 32);        
        sprintf(temp, "%s ", argv[i]);

        strncat(arguments, temp, strlen(argv[i]) + 1);
        free(temp);
    }

    /* O fifo de escrita já está aberto pelo servidor, só precisamos de escrever. */

    int client_to_server;
    const char *cts_fifo = "com/cts";
    client_to_server = open(cts_fifo, O_WRONLY);

    int server_to_client;
    const char *stc_fifo = "com/stc";
    server_to_client = open(stc_fifo, O_RDONLY);

    if (client_to_server < 0 || server_to_client < 0)
    {
        print_error("Could not open at least one FIFO file (cts|stc).\n");
        return OPEN_ERROR;
    }

    /* Enviar a string que contem os argumentos (str) */
    int args_len = strlen(arguments) + 1;

    // if (write(client_to_server, &args_len, sizeof(int)) < 0) raise_write_error();  /* Primeiro enviar o tamanho da string */
    if (write(client_to_server, arguments, args_len) < 0) raise_write_error(); /* E depois a string */
    
    free(arguments);

    /* Acaba a comunicação por isso fechamos os FIFOS do lado do client. */
    close(client_to_server);

    int message;
    while(true)
    {
        if (read(server_to_client, &message, sizeof(int)) < 0) 
        {
            print_error("Could not read from FIFO. <stc in client.c>\n");
            return READ_ERROR;
        }
        
        switch (message)
        {
            case 0:
                print_info("Pending...\n");
                break;

            case 1:
                print_info("Queued up...\n");
                break;

            case 2: 
                print_info("Executing...\n");
                break;

            case 3:
                print_info("Finished!\n");
                break;

            case 4: // Help
                char help_menu[BUFSIZ];
                if (read(server_to_client, help_menu, BUFSIZ) < 0)
                {
                    print_error("Could not read from FIFO. <stc> in client.c");
                    return READ_ERROR;
                }

                if (write(STDOUT_FILENO, help_menu, strlen(help_menu)) < 0)
                {
                    print_error("Could not write to STDOUT_FILENO.");
                    return WRITE_ERROR; 
                }

                return EXIT_SUCCESS;

            default:
                print_error("Unknown message code.\n");
                return UNKNOWN_MESSAGE_ERROR;
        }                
    }

    /* Closes communication between server --> client */
    close(server_to_client);

    return 0;
}

