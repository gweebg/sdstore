#pragma once

/**
 * @brief Status enum that describes the progress of a job.
 * @param PENDING The job was read but not yet queued to be executed.
 * @param QUEUED The job has been queued and it's waiting to be executed.
 * @param EXECUTING The job is being currently executed.
 * @param COMPLETED The job has finished executing and it's output is available.
 */
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
 * @param from Input path.
 * @param to Output path.
 * @param priority Priority of the job.
 * @param status Enum with the current status of the job.
 * @param desc Description string of the job.
 * @param id Job id.
 * @param valid Boolean value that represents whether a job is valid or not.
 * @param op_len Number of operations of the job.
 */
typedef struct input 
{
    char **operations;

    char *from;
    char *to;
    char *desc;

    Status status;

    int id;
    int priority;
    int op_len;
    int valid;
} Input;
