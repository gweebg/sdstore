/**
 * @file utils.c
 * @author gweebg ; johnny_longo 
 * @brief Utilities module with some helper functions. 
 * @version 0.1
 * @date 2022-04-22
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
#include <time.h>
#include <sys/time.h>

#include "../includes/utils.h"

/**
 * @brief A better version of malloc that removes the work of checking for error->
 *
 * @param size Number of bytes to allocate.
 * @return Address of the new allocated memory block.
 */
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
 * @brief Returns the number of operations on an input string.
 * 
 * @param string Input string.
 * @return Number of operations (int).
 */
int total_operations(char *string)
{
    /* stc_19284 proc-file -p 5 tests/in1.txt tests/out1.txt nop bcompress encrypt */   
    char *tok = strtok(string, " ");

    int i = 0, expecting = 4;
    while(tok != NULL) 
    {
        if (strcmp(tok, "-p") == 0) expecting = 6;

        i++;
        tok = strtok(NULL, " \n");
    }

    return i - expecting;
}

/**
 * @brief Makes use of the 'write' function to read a line from a given file descriptor, 
 * because we really are masochists.
 * 
 * @param fd File (aka. file descriptor (integer)).
 * @param line Output parameter containing the read line.
 * @param size Expected size of the line.
 * @return Number of read bytes. 
 */
ssize_t read_line(int fd, char* line, size_t size)
{
    ssize_t bytes_read = read(fd, line, size);
    if (!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;
    if (bytes_read < (ssize_t)line_length) line_length = bytes_read;
    line[line_length] = 0;
    
    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}

/**
 * @brief Using the 'read_line' function reads six lines from the configuration file and
 * generates and populates a Configuration object.
 * 
 * @param path Path from where the configuration file is.
 * @return The configuration struct fully populated.
 */
Configuration generate_config(char *path)
{
    Configuration result;
    int conf_file = open(path, O_RDONLY);

    if (conf_file == -1)
    {
        write(STDERR_FILENO, "[!] Could not read configuration file.\n", 40);
        exit(OPEN_ERROR);
    }

    char *current_line = xmalloc(sizeof(char) * 64);
    char *rest = xmalloc(sizeof(char) * 64);
    for (int i = 0; i < 7; i++)
    {
        read_line(conf_file, current_line, 64);
        char *operation = strtok_r(current_line, " ", &rest);
        int max = atoi(rest);

        if (strcmp(operation, "nop") == 0) result.nop = max;
        else if (strcmp(operation, "bcompress") == 0) result.bcompress = max;
        else if (strcmp(operation, "bdecompress") == 0) result.bdecompress = max;
        else if (strcmp(operation, "gcompress") == 0) result.gcompress = max;
        else if (strcmp(operation, "gdecompress") == 0) result.gdecompress = max;
        else if (strcmp(operation, "encrypt") == 0) result.encrypt = max;
        else if (strcmp(operation, "decrypt") == 0) result.decrypt = max;
        else
        {
            write(STDERR_FILENO, "[!] Invalid configuration file.\n", 33);
            exit(FORMAT_ERROR);
        }
    }

    free(current_line);
    close(conf_file);

    return result;
}

/**
 * @brief Function that sends to the client the usage menu of the programm.
 * 
 * @param server_to_client Named pipe where to write.
 */
void send_help_message(int server_to_client)
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

    if (write(server_to_client, help_menu, strlen(help_menu) + 1) < 0)
    {
        print_error("Could not write into FIFO. <stc> in server.c\n");
        _exit(WRITE_ERROR);
    }
}

/**
 * @brief Prints on the STDOUT_FILENO the help menu for the server.
 */
void print_server_help()
{
    write(STDOUT_FILENO,
          "usage: ./server config-file tools\n"
          "Listen to requests from the client as jobs and execute them.\n"
          "Arguments:\n"
          "config-file    : path to the configuration file for the max capacity for each operation (example file 'config.conf' bellow)\n\n"
          "                         nop 10\n"
          "                         bcompress 10\n"
          "                         bdecompress 10\n"
          "example 'config.conf' :  gcompress 10\n"
          "                         gdecompress 10\n"
          "                         encrypt 10\n"
          "                         decrypt 10\n\n"
          "tools          : path to where the tools nop, bcompress, bdecompress, gcompress, gdecompress, encrypt and decrypt are stored\n"
          "You can run up to 1024 concurrent requests to the server and the queue is updated from 0.2 to 0.2 seconds.\n"
          "Larger files will take longer to process (also depend on the operations).\n"
          ,799);
}

/**
 * @brief Helper function to print out error messages using the 'write' function.
 * 
 * @param content String to print.
 */
void print_error(char *content)
{
    char* temp = malloc(sizeof(char) * (strlen(content) + 8));
    sprintf(temp, "[!] %s", content);
    
    write(STDERR_FILENO, temp, strlen(temp));
    free(temp);
}

/**
 * @brief Helper function to print out info messages using the 'write' function.
 * 
 * @param content String to print.
 */
void print_info(char *content)
{
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    struct tm *time_str_tm = gmtime(&time_now.tv_sec);
    
    char* temp = malloc(sizeof(char) * (strlen(content) + 8) + 16);
    sprintf(temp, "[at %02i:%02i:%02i:%06li] %s", 
    time_str_tm->tm_hour, time_str_tm->tm_min, time_str_tm->tm_sec, time_now.tv_usec, content);
    
    write(STDOUT_FILENO, temp, strlen(temp));
    free(temp);
}

/**
 * @brief Helper function to print out log messages using the 'write' function.
 * 
 * @param content String to print.
 */
void print_log(char *content, int log_file, bool print_to_terminal)
{
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    struct tm *time_str_tm = gmtime(&time_now.tv_sec);
    
    char* temp = malloc(sizeof(char) * (strlen(content) + 8) + 16);
    sprintf(temp, "[DEBUG at %02i:%02i:%02i:%06li] %s", 
    time_str_tm->tm_hour, time_str_tm->tm_min, time_str_tm->tm_sec, time_now.tv_usec, content);

    write(log_file, temp, strlen(temp));

    if (print_to_terminal) write(STDOUT_FILENO, temp, strlen(temp));
    free(temp);
}

/**
 * @brief Generate a string containing the dump of an struct llist.
 * Exclusive for queued up jobs.
 * @param dest Destination string.
 * @param llist List to dump.
 * @param fifo_id Fifo information to be able to send to client.
 */
void generate_status_message_from_queued(char *dest, struct Node *llist, char *fifo_id)
{
    sprintf(dest, "%s\nQueued Up Jobs:\n", fifo_id);

    struct Node *temp = llist;
    if (temp) /* If the list contains elements... */
    {
        int counter = 0;
        while (temp)
        {   
            char *each_job = xmalloc(sizeof(char) * 256);
            sprintf(each_job, "[%d] %s\n", counter, temp->data);

            strcat(dest, each_job);

            temp = temp->next; counter++;
            free(each_job);
        }
    }
    else strcat(dest, "-- no jobs queued up --\n");
}

/**
 * @brief Generate a string containing the dump of an struct llist.
 * Exclusive for in execution jobs.
 * @param dest Destination string.
 * @param llist List to dump.
 */
void generate_status_message_from_executing(char *dest, struct Node *llist)
{
    strcpy(dest, "In Execution Jobs:\n");

    struct Node *temp = llist;
    if (temp)
    {
        int counter = 0;
        while (temp)
        {
            char *current_job = xmalloc(sizeof(char) * 256);
            sprintf(current_job, "[%d] %s\n", counter, temp->data);

            strcat(dest, current_job);
            temp = temp->next; counter++;

            free(current_job);
        }
    }
    else strcat(dest, "-- no jobs currently executing --\n");
}

void generate_completed_message(char *dest, char *in, char *out)
{
    int file_in  = open(in,  O_RDONLY),
        file_out = open(out, O_RDONLY);

    if (file_out < 0 || file_in < 0)
    {
        print_error("Could not files 'from' and 'to'.\n");
        exit(OPEN_ERROR);
    }

    int size_in  = lseek(file_in,  0, SEEK_END),
        size_out = lseek(file_out, 0, SEEK_END);

    sprintf(dest, "[*] Completed (bytes-input: %d, bytes-output: %d)\n", 
            size_in, size_out);
     
    close(file_in);
    close(file_out);
}

void generate_status_message_from_resources(char *dest, int *resources, Configuration config)
{
    sprintf(dest,
            "Resources (using/max):\n"
            "nop:         %d/%d\n"
            "gcompress:   %d/%d\n"
            "gdecompress: %d/%d\n"
            "bcompress:   %d/%d\n"
            "bdecompress: %d/%d\n"
            "encrypt:     %d/%d\n"
            "decrypt:     %d/%d\n",
            resources[0], config.nop,
            resources[1], config.gcompress,
            resources[2], config.gdecompress,
            resources[3], config.bcompress,
            resources[4], config.bdecompress,
            resources[5], config.encrypt,
            resources[6], config.decrypt);
}


void send_status_to_client(char *fifo, char *content)
{
    int server_to_client = open(fifo, O_WRONLY);
    if (server_to_client < 0)
    {
        print_error("Could not files 'server_to_client' @ send_status_to_client.\n");
        exit(OPEN_ERROR);
    }

    if (write(server_to_client, content, strlen(content)) < 0)
    {
        print_error("Could not write content to server_to_client @ send_status_to_client.\n");
        exit(WRITE_ERROR);
    }

    close(server_to_client);
}

/**
 * @brief Checks if there are enough resources to run a job.
 * Does this by checking the 'in_use_operations' array.
 * @param job Job to be checked.
 * @param config Configuration object with the limit values.
 * @return true, if there are enough resources, false otherwise.
 */
bool check_execute(int *job, Configuration config, int *in_use_operations)
{
    /* Checking for excess resources */
    if (job[0] + in_use_operations[0] > config.nop)         return false;
    if (job[1] + in_use_operations[1] > config.gcompress)   return false;
    if (job[2] + in_use_operations[2] > config.gdecompress) return false;
    if (job[3] + in_use_operations[3] > config.bcompress)   return false;
    if (job[4] + in_use_operations[4] > config.bdecompress) return false;
    if (job[5] + in_use_operations[5] > config.encrypt)     return false;
    if (job[6] + in_use_operations[6] > config.decrypt)     return false;

    return true;
}

void update_resources_usage_add(int *resources, Job job_to_execute)
{
    /* 0:nop 1:gcompress 2:gdecompress 3:bcompress 4:bdecompress 5:encrypt 6:decrypt */
    for (int i = 0; i < job_to_execute.op_len; i++)
    {
        if      (strcmp(job_to_execute.operations[i], "./tools/nop"         ) == 0) resources[0]++;
        else if (strcmp(job_to_execute.operations[i], "./tools/gcompress"   ) == 0) resources[1]++;
        else if (strcmp(job_to_execute.operations[i], "./tools/gdecompress" ) == 0) resources[2]++;
        else if (strcmp(job_to_execute.operations[i], "./tools/bcompress"   ) == 0) resources[3]++;
        else if (strcmp(job_to_execute.operations[i], "./tools/bdecompress" ) == 0) resources[4]++;
        else if (strcmp(job_to_execute.operations[i], "./tools/encrypt"     ) == 0) resources[5]++;
        else resources[6]++;
    }
}

void update_resources_usage_del(int *resources, Job job_to_execute)
{
    for (int i = 0; i < job_to_execute.op_len; i++)
    {
        if      (strcmp(job_to_execute.operations[i], "./tools/nop"         ) == 0) resources[0]--;
        else if (strcmp(job_to_execute.operations[i], "./tools/gcompress"   ) == 0) resources[1]--;
        else if (strcmp(job_to_execute.operations[i], "./tools/gdecompress" ) == 0) resources[2]--;
        else if (strcmp(job_to_execute.operations[i], "./tools/bcompress"   ) == 0) resources[3]--;
        else if (strcmp(job_to_execute.operations[i], "./tools/bdecompress" ) == 0) resources[4]--;
        else if (strcmp(job_to_execute.operations[i], "./tools/encrypt"     ) == 0) resources[5]--;
        else                                                                        resources[6]--;                                                                
    }
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

void get_job_resources(Job job, int *resources)
{
    for (int i = 0; i < job.op_len; i++)
    {
        if      (strcmp(job.operations[i], "./tools/nop"         ) == 0) resources[0]++;
        else if (strcmp(job.operations[i], "./tools/gcompress"   ) == 0) resources[1]++;
        else if (strcmp(job.operations[i], "./tools/gdecompress" ) == 0) resources[2]++;
        else if (strcmp(job.operations[i], "./tools/bcompress"   ) == 0) resources[3]++;
        else if (strcmp(job.operations[i], "./tools/bdecompress" ) == 0) resources[4]++;
        else if (strcmp(job.operations[i], "./tools/encrypt"     ) == 0) resources[5]++;
        else resources[6]++;
    }
}