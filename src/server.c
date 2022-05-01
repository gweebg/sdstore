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
#include "../includes/execute.h"

/* Global Variables */
int in_use_operations[7] = {0}; /* 0:nop 1:gcompress 2:gdecompress 3:bcompress 4:bdecompress 5:encrypt 6:decrypt */
Input in_execution_jobs[1024] = {0};
int active_jobs = 0, job_number = 0;
bool queue_in_use = false;

/**
 * @brief A function that parses 'in_use_operations' and 'in_execution_jobs' array into a string
 * and displays as status.
 * @param config Configuration struct containing operation limits.
 * @return The generated status string (char*).
 */
/*
char *generate_status(Configuration config)
{
    char *operation_status = xmalloc(sizeof(char) * 512);
    sprintf(operation_status, "resources status (now/max):\n" 
                              "operation nop: %d/%d\n"
                              "operation gcompress: %d/%d\n"
                              "operation gdecompress: %d/%d\n"
                              "operation bcompress: %d/%d\n"
                              "operation bdecompress: %d/%d\n"
                              "operation encrypt: %d/%d\n"
                              "operation decrypt: %d/%d\n", 
                              in_use_operations[0], config.nop,
                              in_use_operations[1], config.gcompress,
                              in_use_operations[2], config.gdecompress,
                              in_use_operations[3], config.bcompress,
                              in_use_operations[4], config.bdecompress,
                              in_use_operations[5], config.encrypt,
                              in_use_operations[6], config.decrypt);

    char *job_status = xmalloc(sizeof(char) * 512);
    strncpy(job_status, "job status (job_id (status_code): job):\n", 27);   

    if (active_jobs != 0)
        for (int i = 0; i < active_jobs; i++)
        {
            char *temp = xmalloc(sizeof(char) * 32);        
            sprintf(temp, "job #%d (%d): %s\n", in_execution_jobs[i].id,
                                                in_execution_jobs[i].status,
                                                in_execution_jobs[i].desc);

            strncat(job_status, temp, strlen(temp) + 1);
            free(temp);
        }   

    char result[BUFSIZ];
    sprintf(result, "%s%s", job_status, operation_status);
    return result;      
}
*/

/**
 * @brief Checks if there are enough resources to run a job.
 * Does this by checking the 'in_use_operations' array.
 * @param job Job to be checked.
 * @param config Configuration object with the limit values.
 * @return true, if there are enough resources, false otherwise.
 */
bool check_resources(Input job, Configuration config)
{
    Configuration num_operations_per_type = {0}; /* Set all values to 0. */

    /* Counting the number of operations of the job */
    for (int i = 0; i < job.op_len; i++)
    {
        if (strcmp(job.operations[i], "nop") == 0) num_operations_per_type.nop++;
        else if (strcmp(job.operations[i], "gcompress") == 0) num_operations_per_type.gcompress++;
        else if (strcmp(job.operations[i], "gdecompress") == 0) num_operations_per_type.gdecompress++;
        else if (strcmp(job.operations[i], "bcompress") == 0) num_operations_per_type.bcompress++;
        else if (strcmp(job.operations[i], "bdecompress") == 0) num_operations_per_type.bdecompress++;
        else if (strcmp(job.operations[i], "encrypt") == 0) num_operations_per_type.encrypt++;
        else num_operations_per_type.decrypt++;
    }

    /* Checking for excess resources */
    if (num_operations_per_type.nop + in_use_operations[0] > config.nop) return false;
    if (num_operations_per_type.gcompress + in_use_operations[1] > config.gcompress) return false;
    if (num_operations_per_type.gdecompress + in_use_operations[2] > config.gdecompress) return false;
    if (num_operations_per_type.bcompress + in_use_operations[3] > config.bcompress) return false;
    if (num_operations_per_type.bdecompress + in_use_operations[4] > config.bdecompress) return false;
    if (num_operations_per_type.encrypt + in_use_operations[5] > config.encrypt) return false;
    if (num_operations_per_type.decrypt + in_use_operations[6] > config.decrypt) return false;

    return true;
}

/**
 * @brief Create a PreProcessedInput object.
 * 
 * @param string Input string to be 'converted' to a PreProcessedInput struct.
 * @return PreProcessedInput 
 */
PreProcessedInput create_ppinput(char *string)
{
    PreProcessedInput p = {.valid = 1, 
                           .id = job_number, 
                           .desc = strdup(string), 
                           .status = PENDING};
    
    char *token = strtok(string, " ");
    p.fifo = strdup(token);

    token = strtok(NULL, " ");
    if (strcmp(token, "help") == 0)
    {
        p.status = HELP;
        return p;
    }
    else if (strcmp(token, "status") == 0)
    {
        p.status = STATUS;
        return p;
    }

    token = strtok(NULL, " ");
    if (strcmp(token, "-p") == 0)
    {
        token = strtok(NULL, " ");
        p.priority = atoi(token);
    }
    else p.priority = 0;

    if (p.priority < 0 || p.priority > 5)
    {
        print_error("Invalid priority value.\n");
        p.valid = -1;
    }

    job_number++;
    return p;
}

/**
 * @brief Populates an Input struct when given a valid string.
 * 
 * @param string Input string to be 'converted' to a Input struct.
 * @param exec_path Path where the custom (or not) executables are.
 */
Input create_input(PreProcessedInput base, char *exec_path)
{
    Input r;

    r.id = base.id;
    r.status = EXECUTING;
    r.valid = base.valid;
    r.desc = strdup(base.desc);

    r.op_len = get_commands_len(strdup(base.desc));

    char *argument = strtok(base.desc, " ");

    /* Setting priority */
    argument = strtok(NULL, " ");
    r.priority = base.priority;

    /* Setting input path */
    argument = strtok(NULL, " ");
    r.from = strdup(argument);

    /* Setting output path */
    argument = strtok(NULL, " ");
    r.to = strdup(argument);

    /* Setting operations */
    r.operations = xmalloc(sizeof(char) * r.op_len * 16); 
    
    int i = 0;
    argument = strtok(NULL, " ");
    while(argument != NULL) 
    {
        char *temp = xmalloc(sizeof(char) * (strlen(exec_path) + strlen(argument) + 1));
        sprintf(temp, "%s/%s", exec_path, argument);

        r.operations[i++] = strdup(temp);

        free(temp);
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

    /* Opening communication with the client (client to server). */
    int client_to_server;
    const char *cts_fifo = "tmp/cts";
    mkfifo(cts_fifo, 0666);

    print_log("Server is online!\n");
    print_log("Listening for data... \n");

    client_to_server = open(cts_fifo, O_RDONLY);
    if (client_to_server < 0) 
    {
        print_error("Failed to open FIFO <cts in server.c>\n");
        _exit(OPEN_ERROR);
    }

    Configuration config = generate_config(argv[1]);

    PriorityQueue *pqueue = xmalloc(sizeof(PriorityQueue) + sizeof(PreProcessedInput) * QSIZE);
    init_queue(pqueue);

    int input_com[2], dispacher_com[2], handler_com[2];

    if (pipe(input_com) == -1 || pipe(dispacher_com) == -1 || pipe(handler_com) == -1)
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
            
            if (strcmp(arguments, "") != 0) 
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
            !Child Process (Handler)
            Takes a string sent from the pipe (main), parses it and inserts the, newly originated,
            Input struct into the priority queue.
            */

            close(dispacher_com[0]);

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

                PreProcessedInput current_job = create_ppinput(input_string);
                
                if (current_job.valid == 1)
                {
                    int server_to_client = open(current_job.fifo, O_WRONLY);
                    if (server_to_client < 0)
                    {
                        print_error("Could not open server to client fifo.\n");
                        _exit(OPEN_ERROR);
                    }

                    switch(current_job.status)
                    {
                        case HELP:
                            print_info("Help on the way!\n");
                            send_help_message(server_to_client);
                            break;

                        case STATUS:
                            print_info("Checking on me huh...\n");
                            // send_status_message(server_to_client);
                            break;

                        default:
                            
                            if (write(dispacher_com[1], &current_job, sizeof(PreProcessedInput)) < 0)
                            {
                                print_error("Failed to write to dispacher_com (default:).\n");
                                _exit(WRITE_ERROR);
                            }
                            
                            /* Update do status para o cliente */

                            char pending_message[] = "[*] Pending...\n";
                            if (write(server_to_client, pending_message, strlen(pending_message)) < 0)
                            {
                                print_error("Failed to write to FIFO <stc in server.c>\n");
                                _exit(WRITE_ERROR);
                            }      
                    }

                    close(server_to_client);
                }

                /* Reset buffer */
                memset(input_string, 0, BUFSIZ);
            }

            close(dispacher_com[1]);
            close(input_com[0]);
            _exit(EXIT_SUCCESS);
        }
        else
        {
            /*
            !Parent Process (Manager/Executer)
            On an infinite loop this process pops the elements from the queue and tries to execute it, 
            according to the in_use_operations array.
            */

            pid_t queue_manager = fork();
            if (queue_manager < 0)
            {
                print_error("Could not fork process (queue_manager).\n");
                _exit(FORK_ERROR);
            }

            if (queue_manager == 0) //! MANAGER
            {
                /* Process responsible for managing the queue (push, pop, is_empty, ...) */
                close(dispacher_com[1]);
                // close(handler_com[0]);

                int bytes_read; char string[BUFSIZ];
                while(true)
                {
                    while((bytes_read = read(dispacher_com[0], string, BUFSIZ)) > 0) 
                    {  
                        if (strncmp(string, "pop", 3) == 0)
                        {
                            if (is_empty(pqueue) == false)
                            {
                                PreProcessedInput poped_job = pop(pqueue);

                                /* Send through handler_com pipe the poped_job. */
                                if (write(dispacher_com[1], &poped_job, sizeof(PreProcessedInput)) < 0)
                                {
                                    print_error("Could not write to handler_com pipe.\n");
                                    _exit(WRITE_ERROR);
                                }
                            }
                        }
                        else /* If didn't read 'pop' then it's a push command. */
                        {
                            if (pqueue->size != QSIZE)
                            {
                                PreProcessedInput received_job;

                                print_info("tenta ler...\n");
                                /* Ler o job do pipe */
                                if (read(dispacher_com[0], &received_job, sizeof(PreProcessedInput)) < 0)
                                {
                                    print_error("Could not read from dispacher_com pipe.\n");
                                    _exit(READ_ERROR);
                                }

                                print_info("leu\n");

                                if (received_job.valid == 1) push(pqueue, received_job);

                                char info_message[256];
                                sprintf(info_message, "Added new job (id:%s) to the queue.\n", received_job.fifo);
                                print_info(info_message);
                            }
                        }
                    }
                }

                close(dispacher_com[0]);
                // close(handler_com[1]);
            }
            else //! EXECUTER
            {
                /* Process responsible for dequeing and executing jobs. */
            }
        }

        wait(NULL); // Espera pelo processo 'child process (dispacher)'.
    }

    wait(NULL); // Espera pelo processo 'child process (main)'.
    return 0;
}