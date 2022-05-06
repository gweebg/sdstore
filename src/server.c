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
#include "../includes/llist.h"

/* Global Variables */
/* 0:nop 1:gcompress 2:gdecompress 3:bcompress 4:bdecompress 5:encrypt 6:decrypt */
int in_use_operations[7] = {0};         

Job in_execution_jobs[1024];
PreProcessedInput queued_jobs[1024];

int active_jobs = 0, 
    job_number = 0;


/**
 * @brief A function that parses 'in_use_operations' and 'in_execution_jobs' array into a string
 * and displays as status.
 * @param config Configuration struct containing operation limits.
 * @return The generated status string (char*).
 */
/*
char *generate_status(Configuration config)
{
    char *operation_status = malloc(sizeof(char) * 512);
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

    char *job_status = malloc(sizeof(char) * 512);
    strncpy(job_status, "job status (job_id (status_code): job):\n", 27);   

    if (active_jobs != 0)
        for (int i = 0; i < active_jobs; i++)
        {
            char *temp = malloc(sizeof(char) * 32);        
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
bool check_resources(Job job, Configuration config)
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
 * @brief Populates an Job struct when given a valid string.
 * 
 * @param base Input string to be 'converted' to a Job struct.
 * @param exec_path Path where the custom (or not) executables are.
 */
Job create_job(char *base, char *exec_path)
{
    /* stc_19284 proc-file -p 5 tests/in1.txt tests/out1.txt nop bcompress encrypt */
    Job job = {.desc = strdup(base),
               .op_len = total_operations(strdup(base))};

    char *token = strtok(base, " "); /* fifo */
    job.fifo = strdup(token);

    token = strtok(NULL, " "); /* job type */
    token = strtok(NULL, " "); /* -p ? */

    if (strcmp(token, "-p") == 0) 
    {
        token = strtok(NULL, " ");
        token = strtok(NULL, " "); 
        
        job.from = strdup(token);
    }
    else job.from = strdup(token);


    token = strtok(NULL, " "); 
    job.to = strdup(token);

    job.operations = malloc(sizeof(char) * job.op_len * 16); 
    int i = 0;

    token = strtok(NULL, " ");
    while(token) 
    {
        char *temp = malloc(sizeof(char) * (strlen(exec_path) + strlen(token) + 1));
        sprintf(temp, "%s/%s", exec_path, token);

        job.operations[i++] = strdup(temp);

        free(temp);
        token = strtok(NULL, " \n");
    }

    return job;
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

    if (argc == 2 && strcmp(argv[1], "help") == 0)
    {
        print_server_help();
        return EXIT_SUCCESS;
    }

    if (argc != 3)
    {
        print_error("Not enough arguments (expected 2).\n");
        return FORMAT_ERROR;
    }

    /* Opening communication with the client (client to server). */
    int client_to_server;
    const char *cts_fifo = "tmp/cts";
    mkfifo(cts_fifo, 0666);

    print_info("Server is online!\n");

    Configuration config = generate_config(argv[1]);
    // print_config(config);
    
    PriorityQueue *pqueue = malloc(sizeof(PriorityQueue) + sizeof(PreProcessedInput) * QSIZE);
    init_queue(pqueue);

    print_info("Listening for data... \n");

    client_to_server = open(cts_fifo, O_RDONLY);
    if (client_to_server < 0) 
    {
        print_error("Failed to open FIFO <cts in server.c>\n");
        _exit(OPEN_ERROR);
    }

    int log_file = open("logs/log.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (log_file < 0)
    {
        print_error("Failed to open log file.\n");
        _exit(OPEN_ERROR);
    }

    int input_com[2], dispacher_com[2];

    if (pipe(input_com) == -1 || pipe(dispacher_com) == -1)
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

                char *stc_fifo = malloc(sizeof(char) * 64);
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
                        print_log("Help requested.\n", log_file, false);
                        send_help_message(server_to_client);
                        break;

                    case STATUS:
                        print_log("Status requested.\n", log_file, false);

                        int stat_message = STAT; 
                        if (write(input_com[1], &stat_message, sizeof(int)) < 0)
                        {
                            write(STDERR_FILENO, "Something went wrong while writing to pipe.\n", 45);
                            _exit(WRITE_ERROR);
                        }

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

            /* Queued Jobs Struct */
            struct Node *queued_jobs = NULL;

            char input_string[BUFSIZ];
            while (true)
            {
                int size;

                if (read(input_com[0], &size, sizeof(int)) < 0)
                {
                    print_error("Something went wrong while reading from pipe.\n");
                    _exit(READ_ERROR);
                }

                if (size == EMPTY) /* Get status of the queue. */
                {
                    // print_log("Status request received (queue_manager).\n", log_file, false);

                    char *status = is_empty(pqueue) ? "true" : "false";
                    if (write(dispacher_com[1], status, strlen(status)) < 0)
                    {
                        print_error("Could not write to server to client fifo.\n");
                        _exit(WRITE_ERROR);
                    }
                }
                else if (size == POP) /* Pop an element from the queue. */
                {
                    PreProcessedInput job_to_send = pop(pqueue);
                    
                    char *pop_string = xmalloc(sizeof(char) * 128);
                    sprintf(pop_string, "Pop request received from job %s (queue manager).\n", job_to_send.fifo);
                    print_info(pop_string);
                    free(pop_string);

                    int message_length = strlen(job_to_send.desc);
                    if (write(dispacher_com[1], &message_length, sizeof(int)) < 0)
                    {
                        print_error("Could not write to server to client fifo.\n");
                        _exit(WRITE_ERROR);
                    }

                    if (write(dispacher_com[1], job_to_send.desc, message_length) < 0)
                    {
                        print_error("Could not write to server to client fifo.\n");
                        _exit(WRITE_ERROR);
                    }

                    /* Using the PreProcessedInput id parameter, find the job and remove it from the queued_jobs list */
                    llist_delete(&queued_jobs, job_to_send.fifo);
                }
                else if (size == STAT)
                {
                    char *status_string = xmalloc(sizeof(char) * BUFSIZ);

                    struct Node *temp = queued_jobs;
                    while (temp)
                    {
                        char *current_job = xmalloc(sizeof(char) * BUFSIZ);        
                        sprintf(temp, "[Job @ %s - QUEUED] %s",temp->data.fifo, temp->data.desc);

                        strncat(status_string, current_job, strlen(current_job) + 1);
                        free(temp);

                        temp = temp->next;
                    }

                    free(status_string);
                }
                else /* Push an element to the queue. */
                {
                    if (read(input_com[0], input_string, sizeof(char) * size) < 0)
                    {
                        print_error("Something went wrong while reading from pipe.\n");
                        _exit(READ_ERROR);
                    }

                    print_log("Push requested received (queue_manager).\n", log_file, false);
                    PreProcessedInput job = create_ppinput(input_string);
                    if (job.valid) push(pqueue, job);

                    /* Put queued job onto the queued_jobs structure. */
                    llist_push(&queued_jobs, job);

                    char *push_string = xmalloc(sizeof(char) * 128);
                    sprintf(push_string, "Push request received from job %s (queue manager).\n", job.fifo);
                    print_info(push_string);
                    free(push_string);

                    int server_to_client = open(job.fifo, O_WRONLY);
                    if (server_to_client < 0)
                    {
                        print_error("Could not open server to client fifo.\n");
                        _exit(OPEN_ERROR);
                    }

                    char *queued_messase = "[*] Job queued...\n";
                    if (write(server_to_client, queued_messase, strlen(queued_messase)) < 0)
                    {
                        print_error("Could not write to server to client fifo.\n");
                        _exit(WRITE_ERROR);
                    }

                    close(server_to_client);
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

            /* Executing Jobs Struct */
            struct Node *executing_jobs = NULL;

            while(true)
            {
                /* 1º ver se a queue tem elementos. */
                /*    -> mandar para o input_com[0] 'status' e ler número */
                /* 2º mandar para input_com[0] 'pop' e guardar elemento */
                /* 3º executar job */

                int status_message = EMPTY;
                if (write(input_com[1], &status_message, sizeof(int)) < 0)
                {
                    print_error("Could not write to input_com.\n");
                    _exit(WRITE_ERROR);
                }

                char status[BUFSIZ];
                if (read(dispacher_com[0], status, BUFSIZ) < 0)
                {
                    print_error("Could not read from dispacher_com.\n");
                    _exit(READ_ERROR);
                }

                if (strncmp(status, "false", 4) == 0)
                {
                    int pop_message = POP;
                    if (write(input_com[1], &pop_message, sizeof(int)) < 0)
                    {
                        print_error("Could not write to input_com.\n");
                        _exit(WRITE_ERROR);
                    }

                    int message_length;
                    if (read(dispacher_com[0], &message_length, sizeof(int)) < 0)
                    {
                        print_error("Could not read from dispacher_com.\n");
                        _exit(READ_ERROR);
                    }

                    char message[message_length];
                    if (read(dispacher_com[0], message, message_length) < 0)
                    {
                        print_error("Could not read from dispacher_com.\n");
                        _exit(READ_ERROR);
                    }

                    Job to_execute = create_job(message, argv[2]);

                    bool has_to_wait = true;
                    while (has_to_wait)
                    {
                        if (check_resources(to_execute, config))
                        {
                            has_to_wait = false;

                            pid_t new_job = fork();
                            if (new_job < 0)
                            {
                                print_error("Could not fork process (context: executer).\n");
                                _exit(FORK_ERROR);
                            }

                            if (new_job == 0)
                            {
                                print_info("JOB RUNNING\n");

                                struct stat *input_stat = xmalloc(sizeof(struct stat));
                                stat(to_execute.from, input_stat);

                                struct stat *out_stat = xmalloc(sizeof(struct stat));
                                stat(to_execute.to, input_stat);

                                char exit_message[128];
                                sprintf(exit_message, "[*] Completed (bytes-input: %ld, bytes-output: %ld)\n", 
                                        input_stat->st_size, out_stat->st_size);

                                execute(to_execute);

                                int server_to_client = open(to_execute.fifo, O_WRONLY);
                                if (server_to_client < 0)
                                {
                                    print_error("Could not open server to client pipe.\n");
                                    _exit(OPEN_ERROR);
                                }

                                if (write(server_to_client, exit_message, strlen(exit_message)) < 0)
                                {
                                    print_error("Could not write to server to client pipe.\n");
                                    _exit(WRITE_ERROR);
                                }

                                close(server_to_client);

                                char *exec_string = xmalloc(sizeof(char) * 128);
                                sprintf(exec_string, "Executed job (%s).\n", to_execute.fifo);
                                print_info(exec_string);
                                free(exec_string);

                                /* Write to client. */
                                _exit(EXIT_SUCCESS);
                            }

                            // wait(NULL);
                        }
                    }

                    /* Processar string com o job. */
                    /*
                        1º Converter a string em (Input)
                        2º Verificar se ha recursos disponiveis
                            | sim: update resources, executa job, update resources
                            | nao: espera que haja recursos, executa job
                        3º Enviar mensagem de status ao cliente
                    */
                }
                else if (strncmp(status, "[Job", 4) == 0)
                {
                    /* Then we received a status message! */
                    char *status_half = xmalloc(sizeof(char) * BUFSIZ);

                    if (read(dispacher_com[1], status_half, BUFSIZ) < 0)
                    {
                        print_error("Could not read from dispacher_com.\n");
                        _exit(READ_ERROR);
                    }

                    free(status_half);
                }

                /* Let's not spamm it with perma requests. */
                sleep(0.1);
            }

            close(dispacher_com[0]);
            close(input_com[1]);
        }
    }

    wait(NULL); // Espera pelo processo 'child process (main)'.
    close(log_file);
    return 0;
}