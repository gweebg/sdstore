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
    fprintf(stderr, "[!] Could not write to FIFO.\n");
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
        if (!(argc == 2 && (strcmp(argv[1], "status") == 0)))
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
    close(server_to_client);

    return 0;
}

