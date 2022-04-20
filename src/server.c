/**
 * @file server->c
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
#include <sys/wait.h>

#include "../includes/server.h"
#include "../includes/utils.h"
#include "../includes/queue.h"

/**
 * @brief Populates an Input struct when given a valid string.
 * 
 * @param string Input string.
 * @param r Struct that will be populated.
 */
Input create_input(char *string)
{
    Input r;

    int commands_len = get_commands_len(strdup(string));
    r.op_len = commands_len;

    char *argument = strtok(string, " ");
    r.priority = atoi(argument);

    if (r.priority < 0 || r.priority > 5)
    {
        write(STDERR_FILENO, "[!] Invalid priority value.\n", 29);
        exit(FORMAT_ERROR);
    }

    argument = strtok(NULL, " ");
    r.from = strdup(argument);

    argument = strtok(NULL, " ");
    r.to = strdup(argument);

    r.operations = xmalloc(sizeof(char) * commands_len * 16); 
    
    int i = 0;
    argument = strtok(NULL, " ");
    while(argument != NULL) 
    {
        r.operations[i++] = strdup(argument);
        argument = strtok(NULL, " \n");
    }

    return r;
}

int main(int argc, char *argv[])
{
    /*
    argv[0]: executable name
    argv[1]: configuration file
    argv[2]: operations directory

    fd[0] - read
    fd[1] - write
    */

    if (argc != 3)
    {
        write(STDERR_FILENO, "[!] Not enough arguments (expected 2).\n", 40);
        return FORMAT_ERROR;
    }

    Configuration config = generate_config(argv[1]);

    int input_com[2];

    if (pipe(input_com) == -1)
    {
        printf("Something went wrong while creating the pipe.\n");
        return PIPE_ERROR;
    }

    pid_t pid_main = fork();
    if (pid_main < 0)
    {
        printf("Something went wrong while creating a new process.\n");
        return FORK_ERROR;
    }

    if (pid_main == 0)
    {
        /* 
        Child Process
        Listener dos pedidos enviados pelo servidor e tambem, atraves de um pipe, envia o pedido
        para o processo pai para que este seja armazenado de acordo com a prioridade numa priority
        queue. 
        */

        close(input_com[0]);

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
        //? ./nop < in.txt | ./encrypt | ./gcompress | ./nop > out.txt

        char arguments[BUFSIZ];
        while(true)
        {
            /* 
            Ler o valor dos argumentos enviados pelo client (str) (primeiro o tamanho e depois a string)
            int args_len, read_bytes;
            if ((read_bytes = read(client_to_server, &args_len, sizeof(int))) < 0) raise_read_error();
            else if (read_bytes != 0) printf("[>] %d\n", args_len);
            */

            // TODO Para tornar mais eficiente convém mandar primeiro o tamanho da string.
            if (read(client_to_server, arguments, BUFSIZ) < 0) 
            {
                fprintf(stderr, "[!] Could not read from FIFO.\n");
                _exit(READ_ERROR);
            }
            else if (strcmp(arguments, "") != 0) 
            {
                // printf("[sent>] %s\n\n", arguments);

                /* 
                Se a string recebida não for vazia, então enviamos a string através de um pipe
                para outro processo para o seu parsing e futuro armazenamento na queue.
                */

                int input_length = strlen(arguments) + 1; 

                if (write(input_com[1], &input_length, sizeof(int)) < 0)
                {
                    printf("Something went wrong while writing to pipe.\n");
                    _exit(WRITE_ERROR);
                }

                if (write(input_com[1], arguments, sizeof(char) * input_length) < 0)
                {
                    printf("Something went wrong while writing to pipe.\n");
                   _exit(WRITE_ERROR);
                }
            }

            /* Reset buffer */
            memset(arguments, 0, BUFSIZ);
        }

        close(client_to_server);
        close(server_to_client);
        close(input_com[1]);

        /*
        unlink(cts_fifo);
        unlink(stc_fifo);
        */

        _exit(EXIT_SUCCESS);
    }
    else
    {
        /* 
        Parent Process 
        Recebe os pedidos pelo pipe do processo filho faz o parsing e armazena numa priority queue.
        Todas as string recebidas neste lado do pipe são não vazias.
        */
       
        char input_string[BUFSIZ];
        while (true)
        {
            close(input_com[1]);
            int size;

            if (read(input_com[0], &size, sizeof(int)) < 0)
            {
                printf("Something went wrong while reading from pipe.\n");
                _exit(READ_ERROR);
            }

            if (read(input_com[0], input_string, sizeof(char) * size) < 0)
            {
                printf("Something went wrong while reading from pipe.\n");
                _exit(READ_ERROR);
            }

            //TODO Parsing da input_string e adicionar na queue.

            /* Reset buffer */
            memset(input_string, 0, BUFSIZ);
        }
        
        close(input_com[0]);
        _exit(EXIT_SUCCESS);
    }

    wait(NULL);
    return 0;
}