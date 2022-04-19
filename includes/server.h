#pragma once

#define READ_ERROR 1
#define FORK_ERROR 2
#define PIPE_ERROR 3
#define WRITE_ERROR 4

#define MAX_PROC 8

/**
 * @brief Input data structure.
 * @param operations Array containing every operation to be executed on the file.
 * @param proc_file Whether the file is being encrypted or not.
 * @param from Input path.
 * @param to Output path.
 * @param priority Priority of the job.
 */
typedef struct input 
{
    char **operations;

    char *from;
    char *to;

    int priority;
    int op_len;

    bool proc_file;

} Input;
