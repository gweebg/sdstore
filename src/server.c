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
#include <sys/wait.h>

#include "../includes/server.h"
#include "../includes/utils.h"
#include "../includes/queue.h"

/* Global Variables */
int in_use_operations[6];


/**
 * @brief Populates an Input struct when given a valid string.
 * @param string Input string to be 'converted' to a Input struct.
 */
Input create_input(char *string)
{
    Input r;

    r.valid = 1;
    r.op_len = get_commands_len(strdup(string));;

    char *argument = strtok(string, " ");

    argument = strtok(NULL, " ");
    r.priority = atoi(argument);

    if (r.priority < 0 || r.priority > 5)
    {
        print_error("Invalid priority value.\n");
        r.valid = -1;
    }

    argument = strtok(NULL, " ");
    r.from = strdup(argument);

    argument = strtok(NULL, " ");
    r.to = strdup(argument);

    r.operations = xmalloc(sizeof(char) * r.op_len * 16); 
    
    int i = 0;
    argument = strtok(NULL, " ");
    while(argument != NULL) 
    {
        r.operations[i++] = strdup(argument);
        argument = strtok(NULL, " \n");
    }

    return r;
}


/**
 * @brief Funtion that executes the whole server side.
 * Handles client jobs and the configuration files.
 * 
 * @param argc Number or arguments.
 * @param argv Array containing command line arguments.
 * @return Error code (int).
 */
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
        print_error("Not enough arguments (expected 2).\n");
        return FORMAT_ERROR;
    }

    /* cts <=> client_to_server */
    int client_to_server;
    const char *cts_fifo = "com/cts";
    mkfifo(cts_fifo, 0666);

    /* stc <=> server_to_client */
    int server_to_client;
    const char *stc_fifo = "com/stc";
    mkfifo(stc_fifo, 0666);

    print_log("Server is online!\n");
    print_log("Listening for data... \n");

    client_to_server = open(cts_fifo, O_RDONLY);
    if (client_to_server < 0) 
    {
        print_error("Failed to open FIFO <cts in server.c>\n");
        _exit(OPEN_ERROR);
    }

    server_to_client = open(stc_fifo, O_WRONLY);
    if (server_to_client < 0) 
    {
        print_error("Failed to open FIFO <sct in server.c>\n");
        _exit(OPEN_ERROR);
    }

    Configuration config = generate_config(argv[1]);

    PriorityQueue *pqueue = xmalloc(sizeof(PriorityQueue) + 
                                    sizeof(Input) * QSIZE);
    init_queue(pqueue);

    int input_com[2];

    if (pipe(input_com) == -1)
    {
        print_error("Something went wrong while creating the pipe.\n");
        return PIPE_ERROR;
    }

    pid_t pid_main = fork();
    if (pid_main < 0)
    {
        print_error("Something went wrong while creating a new process.\n");
        return FORK_ERROR;
    }

    if (pid_main == 0)
    {
        /* 
        !Child Process (Main)
        Listener dos pedidos enviados pelo servidor e tambem, atraves de um pipe, envia o pedido
        para o processo pai para que este seja armazenado de acordo com a prioridade numa priority
        queue. 
        */

        close(input_com[0]);

        char arguments[BUFSIZ];
        while(true)
        {
            /* 
            Ler o valor dos argumentos enviados pelo client (str) (primeiro o tamanho e depois a string)
            int args_len, read_bytes;
            if ((read_bytes = read(client_to_server, &args_len, sizeof(int))) < 0) raise_read_error();
            else if (read_bytes != 0) printf("[>] %d\n", args_len);
            */

            if (read(client_to_server, arguments, BUFSIZ) < 0) 
            {
                print_error("Could not read from FIFO.\n");
                _exit(READ_ERROR);
            }
            else if (strncmp(arguments, "help", 4) == 0)
            {
                char *help_menu = "usage: ./client [mode] priority input_file output_file [operations]\n"
                                  "Submit jobs to be executed.\n"
                                  "Options and arguments:\n"
                                  "Modes:\n"
                                  "proc-file   : submit a job to the server, requires [0<=priority<=5], [input_file], [output_file] and [operations]\n"
                                  "status      : display a status message containing the status of the server (./client status)\n"
                                  "help        : display this message (./client help)\n"
                                  "Operations:\n"
                                  "nop         : just a nop, does nothing\n"
                                  "gcompress   : compresses the file with the format gzip\n"
                                  "gdecompress : decompresses the file which format is gzip\n"
                                  "bcompress   : compresses the file with the format bzip\n"
                                  "bdecompress : decompresses the file which format is bzip\n"
                                  "encrypt     : encrypts the file (ccrypt)\n"
                                  "decrypt     : decrypts the file (ccrypt)\n"
                                  "Do not forget to start the server application before running a request. Otherwise you will get a deadlock.\n";

                int message = 4;
                if (write(server_to_client, &message, sizeof(int)) < 0)
                {
                    print_error("Could not write into FIFO. <stc> in server.c\n");
                    _exit(WRITE_ERROR);
                }

                if (write(server_to_client, help_menu, strlen(help_menu)) < 0)
                {
                    print_error("Could not write into FIFO. <stc> in server.c\n");
                    _exit(WRITE_ERROR);
                }
            }
            else if (strcmp(arguments, "") != 0) 
            {
                /* 
                Se a string recebida não for vazia, então enviamos a string através de um pipe
                para outro processo para o seu parsing e futuro armazenamento na queue.
                */                
                int input_length = strlen(arguments) + 1; 

                if (write(input_com[1], &input_length, sizeof(int)) < 0)
                {
                    write(STDERR_FILENO, "Something went wrong while writing to pipe.\n", 45);
                    _exit(WRITE_ERROR);
                }

                if (write(input_com[1], arguments, sizeof(char) * input_length) < 0)
                {
                    write(STDERR_FILENO, "Something went wrong while writing to pipe.\n", 45);
                   _exit(WRITE_ERROR);
                }
            }

            /* Reset buffer */
            memset(arguments, 0, BUFSIZ);
        }

        close(client_to_server);
        close(input_com[1]);

        _exit(EXIT_SUCCESS);
    }
    else
    {
        /* 
        !Parent Process (Main)
        Recebe os pedidos pelo pipe do processo filho faz o parsing e armazena numa priority queue.
        Todas as string recebidas neste lado do pipe são não vazias.
        */

        pid_t pid_dispacher = fork();
        if (pid_dispacher < 0)
        {
            print_error("Something went wrong while creating a new process.\n");
            return FORK_ERROR;
        }

        if (pid_dispacher == 0)
        {
            /*
            !Child Process (Dispacher)
            Takes a string sent from the pipe (main), parses it and inserts the, newly originated,
            Input struct into the priority queue.
            */

            char input_string[BUFSIZ];
            while (true)
            {
                close(input_com[1]);
                int size;

                if (read(input_com[0], &size, sizeof(int)) < 0)
                {
                    print_error("Something went wrong while reading from pipe.\n");
                    _exit(READ_ERROR);
                }

                if (read(input_com[0], input_string, sizeof(char) * size) < 0)
                {
                    print_error("Something went wrong while reading from pipe.\n");
                    _exit(READ_ERROR);
                }

                Input current_job = create_input(input_string);
                
                if (current_job.valid == 1)
                {
                    push(pqueue, current_job);
                    print_info("Added a new job to the queue\n");
                    if (write(server_to_client, &(current_job.status), sizeof(int)) < 0)
                    {
                        print_error("Failed to write to FIFO <stc in server.c>\n");
                        _exit(WRITE_ERROR);
                    }
                }

                /* Reset buffer */
                memset(input_string, 0, BUFSIZ);
            }

            close(input_com[0]);
            _exit(EXIT_SUCCESS);
        }
        else
        {
            /*
            !Parent Process (Dispacher)
            On an infinite loop this process pops the elements from the queue and tries to execute it, 
            according to the in_use_operations array.
            */

           
        }

        wait(NULL); // Espera pelo processo 'child process (dispacher)'.
        close(server_to_client);
    }

    wait(NULL); // Espera pelo processo 'child process (main)'.
    return 0;
}