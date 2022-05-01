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

int get_status(char *string, char *fifo_output)
{
    char *token = strtok(string, " ");   
    if (fifo_output)
    {
        strcpy(fifo_output,token);
    }

    token = strtok(NULL, " ");
    if (strcmp(token, "help") == 0) return HELP;
    if (strcmp(token, "status") == 0) return STATUS;
    if (strcmp(token, "proc-file") == 0) return PENDING;

    return -1;
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
        !Receiver
        Listener dos pedidos enviados pelo servidor e tambem, atraves de um pipe, envia o pedido
        para o queue manager para que este seja armazenado de acordo com a prioridade numa priority
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

                char *stc_fifo = xmalloc(sizeof(char) * 64);
                int message_status = get_status(strdup(arguments), stc_fifo);

                int server_to_client = open(stc_fifo, O_WRONLY);
                if (server_to_client < 0)
                {
                    print_error("Could not open server to client fifo.\n");
                    _exit(OPEN_ERROR);
                }

                switch(message_status)
                {
                    case HELP:
                        print_log("Help requested.\n");
                        send_help_message(server_to_client);
                        break;

                    case STATUS:
                        print_log("Status requested.\n");
                        break;

                    case PENDING:

                        char *status_message = "[*] Pending...\n";
                        if (write(server_to_client, status_message, strlen(status_message)) < 0)
                        {
                            write(STDERR_FILENO, "Something went wrong while writing to pipe.\n", 45);
                            _exit(WRITE_ERROR);
                        }

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
                        break;

                    default:
                        break;
                }

                close(server_to_client);
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
        pid_t pid_dispacher = fork();
        if (pid_dispacher < 0)
        {
            print_error("Something went wrong while creating a new process.\n");
            return FORK_ERROR;
        }

        if (pid_dispacher == 0)
        {
            /*
            !Queue Manager
            Takes a string sent from the pipe (Receiver) and inserts the, newly originated,
            Input struct into the priority queue.
            */

            close(input_com[1]);
            close(dispacher_com[0]);

            char input_string[BUFSIZ];
            while (true)
            {
                int size;

                /*! MUDAR ISTO PORQUE PODE DAR DATA TANGLING*/
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

                if (strncmp(input_string, "pop", 3) == 0)
                {
                    PreProcessedInput job = pop(pqueue);

                    if (job.valid == 1)
                    {
                        if (write(dispacher_com[1], job.desc, strlen(job.desc)) < 0)
                        {
                            print_error("Could not write to server toclient fifo.\n");
                            _exit(WRITE_ERROR);
                        }
                    }

                    /* Escrever para o pipe do executer. */
                    print_log("Pop requested.\n");
                }
                else if (strncmp(input_string, "status", 6) == 0)
                {   
                    print_info("Queue status requested.\n");
                    int statuss = is_empty(pqueue) ? LUCKY_NUMBER : -LUCKY_NUMBER;

                    char *status = "vazia";
                    if (write(dispacher_com[1], status, strlen(status)) < 0)
                    {
                        print_error("Could not write to server toclient fifo.\n");
                        _exit(WRITE_ERROR);
                    }

                    /* Escrever para o pipe do executer. */
                    print_log("Queue status requested.\n");
                }
                else
                {
                    PreProcessedInput job = create_ppinput(input_string);
                    if (job.valid) push(pqueue, job);
                    print_log("Push requested.\n");

                    int server_to_client = open(job.fifo, O_WRONLY);
                    if (server_to_client < 0)
                    {
                        print_error("Could not open server to client fifo.\n");
                        _exit(OPEN_ERROR);
                    }

                    char *queued_messase = "[*] Job queued...\n";
                    if (write(server_to_client, queued_messase, strlen(queued_messase)) < 0)
                    {
                        print_error("Could not write to server toclient fifo.\n");
                        _exit(WRITE_ERROR);
                    }
                }

                /* Reset buffer */
                memset(input_string, 0, BUFSIZ);
            }

            close(input_com[0]);
            close(dispacher_com[1]);
            _exit(EXIT_SUCCESS);
        }
        else
        {
            close(dispacher_com[1]);
            close(input_com[0]);

            while(true)
            {
                /* 1º ver se a queue tem elementos. */
                /*    -> mandar para o input_com[0] 'status' e ler número */
                /* 2º mandar para input_com[0] 'pop' e guardar elemento */
                /* 3º executar job */

                char *status_message = "status";
                if (write(input_com[1], status_message, strlen(status_message)) < 0)
                {
                    print_error("Could not write to input_com.\n");
                    _exit(WRITE_ERROR);
                }

                print_log("status sent");

                char status[BUFSIZ];
                if (read(dispacher_com[0], status, BUFSIZ) < 0)
                {
                    print_error("Could not read from dispacher_com.\n");
                    _exit(READ_ERROR);
                }

                printf("status: %s\n", status);
                // if (status == LUCKY_NUMBER)
                // {
                //     print_info("não está empty.\n");
                // }

            }

            close(dispacher_com[0]);
            close(input_com[1]);
        }
    }

    wait(NULL); // Espera pelo processo 'child process (main)'.
    return 0;
}