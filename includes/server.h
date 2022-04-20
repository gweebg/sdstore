#pragma once

#define MAX_PROC 8
#define QSIZE 1024

typedef enum
{
    PENDING,
    QUEUED,
    EXECUTING,
    COMPLETED

} Status;

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

    Status status;

} Input;

void *xmalloc(size_t size);
