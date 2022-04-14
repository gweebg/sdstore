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

#define DEBUG

void *xmalloc(size_t size)
{
    void *result = malloc(size);
    if (!result)
    {
        fprintf(stderr, "Failed to allocate memory (malloc error).\n");
        return NULL;
    }
    return result;
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
    if (argc < 6)
    {
        fprintf(stderr, "Not enought arguments.\nExpected at least 6 but got %d arguments, refer to the documentation for more information.\n", argc);
        return FORMAT_ERROR;
    }

    bool proc_file = false;
    if (strcmp(argv[1], "proc_file") != 0) proc_file = true; 

    char *ptr;
    int priority = strtol(argv[2], &ptr, 10);

    if (*ptr != '\0')
    {
        fprintf(stderr, "Not enought arguments.\nExpected at least 6 but got %d arguments, refer to the documentation for more information.\n", argc);
        return FORMAT_ERROR;
    }

                       /* Usar o xmalloc para não ter de dar handle ao erro. */
    char *input_file = xmalloc(sizeof(char) * (strlen(argv[3]) + 1));
    memcpy(input_file, argv[3], strlen(argv[3]) + 1);

    char *output_file = xmalloc(sizeof(char) * (strlen(argv[4]) + 1));
    memcpy(output_file, argv[4], strlen(argv[4]) + 1);

    int operation_count = argc - 5;
    char *operations = xmalloc(sizeof(char) * 16 * operation_count);

    /* Meter todos os comandos (gcompress, nop, ...) numa string para poder enviar mais facilmente. */
    for (int i = 5; i < argc; i++)
    {
        char *temp = xmalloc(sizeof(char) * 16);        
        sprintf(temp, "%s ", argv[i]);

        strncat(operations, temp, strlen(argv[i]) + 1);
        free(temp);
    }

    #ifdef DEBUG
        printf("ProcFile >> %d\n", proc_file);
        printf("Priority >> %d\n", priority);
        printf("Input    >> %s\n", input_file);
        printf("Output   >> %s\n", output_file);
        printf("Commands >> %s\n", operations);
    #endif

    /* O fifo de escrita já está aberto pelo servidor, só precisamos de escrever. */

    int client_to_server;
    const char *cts_fifo = "com/cts";
    client_to_server = open(cts_fifo, O_WRONLY);

    int server_to_client;
    const char *stc_fifo = "com/stc";
    server_to_client = open(stc_fifo, O_RDONLY);

    if (client_to_server < 0 || server_to_client < 0)
    {
        fprintf(stderr, "[!] Could not open at least one FIFO file (cts|stc).\n");
        return OPEN_ERROR;
    }

    if (write(client_to_server, operations, strlen(operations)) == -1)
    {
        fprintf(stderr, "[!] Could not write to FIFO 'client_to_server'.\n");
        return WRITE_ERROR;
    }
    
    free(input_file); free(output_file);
    free(operations);

    // close(client_to_server);
    // close(server_to_client);

    return 0;
}

