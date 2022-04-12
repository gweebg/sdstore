#pragma once

#define FORMAT_ERROR 1
#define MALLOC_ERROR 2

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
    bool proc_file;

} Input;
