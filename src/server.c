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

   /* If the command is './server help' print the help menu. */
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

    /* Set up of the genereal job queue and config struct containing the max amount of resources. */
    Configuration config = generate_config(argv[1]);
    
    PriorityQueue *pqueue = malloc(sizeof(PriorityQueue) + sizeof(PreProcessedInput) * QSIZE);
    init_queue(pqueue);

    print_info("Listening for data... \n");

    client_to_server = open(cts_fifo, O_RDONLY);
    if (client_to_server < 0) /* Opening cts_fifo */
    {
        print_error("Failed to open FIFO <cts in server.c>\n");
        _exit(OPEN_ERROR);
    }

    int log_file = open("logs/log.txt", O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (log_file < 0) /* Opening log file */
    {
        print_error("Failed to open log file.\n");
        _exit(OPEN_ERROR);
    }

    /* In between processes pipes */
    int input_com[2], dispacher_com[2], job_string[2], pop_com[2];

    if (pipe(input_com) == -1 || pipe(dispacher_com) == -1 || pipe(job_string) == -1 || pipe(pop_com) == -1)
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

    /* 
    !Receiver
    Listener dos pedidos enviados pelo servidor e tambem, atraves de um pipe, envia o pedido
    para o queue manager para que este seja armazenado de acordo com a prioridade numa priority
    queue. 
    */
    if (pid_main == 0)
    {

        close(input_com[0]);
        close(job_string[0]);

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
            
            if (strncmp(arguments, "tmp", 3) == 0) 
            {
                char *stc_fifo = malloc(sizeof(char) * 64);
                int message_status = get_status(strdup(arguments), stc_fifo);

                /* Making sure the '\0' is present to avoid any memory leaks. */
                stc_fifo[strlen(stc_fifo)] = '\0';
                arguments[strlen(arguments)] = '\0';

                /* printf("String: %s\nSize: %ld\n", arguments, strlen(arguments));
                printf("Status: %d\nFifo: %s\n", message_status, stc_fifo);
                printf("Fifo size: %ld\n", strlen(stc_fifo)); */

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
                        break;

                    case PENDING:

                        char *status_message = "[*] Pending...\n";
                        if (write(server_to_client, status_message, strlen(status_message)) < 0)
                        {
                            print_error("Something went wrong while writing to pipe.\n");
                            exit(WRITE_ERROR);
                        }

                        int input_length = strlen(arguments) + 1; 
                        if (write(input_com[1], &input_length, sizeof(int)) < 0)
                        {
                            print_error("Something went wrong while writing to pipe.\n");
                            _exit(WRITE_ERROR);
                        }

                        if (write(job_string[1], arguments, input_length) < 0)
                        {
                            print_error("Something went wrong while writing to pipe.\n");
                            exit(WRITE_ERROR);
                        }

                        print_log("New job received.\n", log_file, false);
                        break;

                    default:
                        break;
                }

                free(stc_fifo);
                close(server_to_client);
            }

            /* Reset buffer */
            memset(arguments, 0, BUFSIZ);
        }

        close(client_to_server);
        close(input_com[1]);
        close(job_string[1]);

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

            close(job_string[1]);
            close(input_com[1]);

            close(dispacher_com[0]);
            close(pop_com[0]);

            /* Queued Jobs Struct */
            struct Node *queued_jobs = NULL;

            while (true)
            {
                int size;
                if (read(input_com[0], &size, sizeof(int)) < 0)
                {
                    print_error("Something went wrong while reading from pipe.\n");
                    _exit(READ_ERROR);
                }

                if (size == EMPTY) /* Get status of the queue (is empty or not). */
                {
                    char *status = is_empty(pqueue) ? "empty" : "false";
                    if (write(dispacher_com[1], status, strlen(status) + 1) < 0)
                    {
                        print_error("Could not write to server to client fifo.\n");
                        _exit(WRITE_ERROR);
                    }
                }
                else if (size == POP) /* Pop an element from the queue. */
                {
                    PreProcessedInput job_to_send = pop(pqueue);

                    char *pop_string = xmalloc(sizeof(char) * (45 + strlen(job_to_send.fifo)));
                    sprintf(pop_string, "Pop request received from job %s (q_manager).\n", job_to_send.fifo);
                    print_info(pop_string);
                    free(pop_string);

                    int message_length = strlen(job_to_send.desc) + 1;
                    if (write(pop_com[1], &message_length, sizeof(int)) < 0)
                    {
                        print_error("Could not write job.desc length (POP reques) to pop_com.\n");
                        _exit(WRITE_ERROR);
                    }

                    if (write(pop_com[1], job_to_send.desc, message_length) < 0)
                    {
                        print_error("Could not write job.desc (POP request) to pop_com.\n");
                        _exit(WRITE_ERROR);
                    }

                    /* Using the PreProcessedInput id parameter, find the job and remove it from the queued_jobs list */
                    llist_delete(&queued_jobs, job_to_send.fifo);
                }
                else if (size == STAT) /* Retrieve informataion about the state of the queue. */
                {
                    print_log("Status message received (q_manager).\n", log_file, false);
                }
                else /* Push element to the stack. */ 
                {
                    if (size > 0)
                    {
                        char job_str[size];
                    
                        if (read(job_string[0], job_str, size) < 0)
                        {
                            print_error("Something went wrong while reading from pipe.\n");
                            _exit(READ_ERROR);
                        }

                        job_str[size] = '\0';

                        print_log("Push requested received (q_manager).\n", log_file, false);

                        PreProcessedInput job = create_ppinput(job_str);
                        
                        // printf("Valid: %d\nPriority: %d\nDesc: %s\nFifo: %s\n", 
                        //        job.valid, job.priority, job.desc, job.fifo);
                        
                        if (job.valid) 
                        {
                            push(pqueue, job);
                            llist_push(&queued_jobs, job.desc);
                        }

                        char *push_string = xmalloc(sizeof(char) * (46 + strlen(job.fifo)));
                        sprintf(push_string, "Push request received from job %s (q_manager).\n", job.fifo);
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
                        memset(job_str, 0, sizeof(job_str));
                    }
                }
            }

            close(dispacher_com[1]);
            close(pop_com[1]);

            close(job_string[0]);
            close(input_com[0]);

            _exit(EXIT_SUCCESS);
        }
        else
        {
            close(dispacher_com[1]);
            close(pop_com[1]);

            close(input_com[0]);

            /* Currently executing jobs. */
            struct Node *executing_jobs = NULL;

            while (true)
            {
                /* Check if the queue is empty or not. */
                /* If it has elements then we pop the queue. */

                int status_message = EMPTY;
                if (write(input_com[1], &status_message, sizeof(int)) < 0)
                {
                    print_error("Could not write 'EMPTY' message to input_com.\n");
                    _exit(WRITE_ERROR);
                }

                char status[6]; /* dispacher_com : sends empty or false to is_empty. */
                if (read(dispacher_com[0], status, 6) < 0)
                {
                    print_error("Could not read 'EMPTY' response from dispacher_com.\n");
                    _exit(READ_ERROR);
                }

                /* If status == 'false' we pop the queue sending a POP message to input_com[0] */
                if (strncmp(status, "false", 5) == 0)
                {
                    int pop_message = POP;
                    if (write(input_com[1], &pop_message, sizeof(int)) < 0)
                    {
                        print_error("Could not write 'POP' to input_com.\n");
                        _exit(WRITE_ERROR);
                    }

                    int response_size; /* size of the response string */
                    if (read(pop_com[0], &response_size, sizeof(int)) < 0)
                    {
                        print_error("Could not read 'EMPTY' response from dispacher_com.\n");
                        _exit(READ_ERROR);
                    }

                    char response_job[response_size];
                    if (read(pop_com[0], response_job, response_size) < 0)
                    {
                        print_error("Could not read 'EMPTY' response from dispacher_com.\n");
                        _exit(READ_ERROR);
                    }

                    Job current_job = create_job(response_job, argv[2]);
                    llist_push(&executing_jobs, current_job.desc);
                    /* Dont need to check for validity because it was already checked on PreProcessedInput. */

                    // printf("From: %s\nTo: %s\n#OP: %d\nFIFO: %s\n", 
                    //         current_job.from, current_job.to, current_job.op_len, current_job.fifo);
                    
                    // for (int i = 0; i < current_job.op_len; i++) printf("%s\n",current_job.operations[i]);

                    bool wait = true;
                    while (wait)
                    {
                        if (check_resources(current_job, config))
                        {
                            wait = false;

                            pid_t exec_fork = fork();
                            if (exec_fork < 0)
                            {
                                print_error("Could not fork process @ executing job.\n");
                                _exit(FORK_ERROR);
                            }

                            if (exec_fork == 0)
                            {
                                print_log("Executing a job.\n", log_file, false);

                                execute(current_job);

                                char *exec_string = xmalloc(sizeof(char) * 128);
                                sprintf(exec_string, "Executed job (%s).\n", current_job.fifo);
                                print_info(exec_string);
                                free(exec_string);

                                char *completed_message = xmalloc(sizeof(char) * 128);
                                generate_completed_message(completed_message, current_job.from, current_job.to);

                                send_status_to_client(current_job.fifo, completed_message);
                                _exit(EXIT_SUCCESS);
                            }
                        }
                    }

                    llist_delete(&executing_jobs, current_job.desc);
                }        

                sleep(1);
            }

            close(dispacher_com[0]);
            close(pop_com[0]);

            close(input_com[1]);
        }
    }

    wait(NULL);
    
    close(log_file);
    return 0;
}

    //             else if (strncmp(status, "stc", 3) == 0)
    //             {
    //                 print_log("status message on executer.\n", log_file, true);

    //                 /* Then we received a status message! */
    //                 char *status_half = xmalloc(sizeof(char) * 1024);
    //                 if (read(dispacher_com[0], status_half, 1024) < 0)
    //                 {
    //                     print_error("Could not read from dispacher_com.\n");
    //                     _exit(READ_ERROR);
    //                 }

    //                 /*
    //                 The read string contains the following format:
    //                 where_to_send_fifo
    //                 queued_job_one
    //                 ...
    //                 queued_job_n
    //                 */

    //                 char *stc_fifo = strtok(status_half, "\n");
    //                 char *second_status_half = xmalloc(sizeof(char) * 1024);

    //                 strcpy(second_status_half, "In Execution Jobs:\n");

    //                 struct Node *temp = executing_jobs;
    //                 int counter = 0;
    //                 while (temp)
    //                 {
    //                     char *current_job = xmalloc(sizeof(char) * 256);
    //                     sprintf(current_job, "[%d] %s\n", counter, temp->data);

    //                     strcat(second_status_half, current_job);
    //                     temp = temp->next; counter++;

    //                     free(current_job);
    //                 }

    //                 char *status = xmalloc(sizeof(char) * BUFSIZ);
    //                 sprintf(status, "[SERVER STATUS]\n%s%s", status_half, second_status_half);

    //                 int server_to_client = open(stc_fifo, O_WRONLY);
    //                 if (server_to_client < 0)
    //                 {
    //                     print_error("Could not open server to client fifo (context: status).\n");
    //                     _exit(OPEN_ERROR);
    //                 }

    //                 if (write(server_to_client, status, strlen(status)) < 0)
    //                 if (server_to_client < 0)
    //                 {
    //                     print_error("Could not write to server to client fifo (context: status).\n");
    //                     _exit(WRITE_ERROR);
    //                 }

    //                 close(server_to_client);

    //                 free(status_half);
    //             }

    //             /* Let's not spamm it with perma requests. */
    //             sleep(0.1);
    //         }

    //         close(dispacher_com[0]);
    //         close(input_com[1]);