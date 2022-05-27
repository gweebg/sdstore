#pragma once

#define POP -22
#define EMPTY -30
#define STAT -27
#define UPDATE_ADD -31
#define UPDATE_DEL -32
#define CHECK_RESOURCES -33

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
    COMPLETED,
    HELP,
    STATUS

} Status;

/**
 * @brief PreProcessedInput struct, a structure that contains a job with only its priority parsed.
 * @param priority The priority of the job.
 * @param id The job id.
 * @param desc The job description in string format (what comes through the named pipe).
 * @param valid Indicates whether the job is valid of not.
 */
typedef struct ppinput
{
    char *desc;
    char *fifo;
    
    int priority, 
        id, 
        valid;

    Status status;

} PreProcessedInput;

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
typedef struct job 
{
    char **operations;

    char *from,
         *to,
         *fifo,
         *desc;

    Status status;

    int op_len;
} Job;
