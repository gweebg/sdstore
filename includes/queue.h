#pragma once

#include "server.h"

/**
 * @brief Priority queue implementation using an array.
 * 
 * @param values Array of Input structs, one for each job.
 * @param size Size of the queue.
 */
typedef struct PriorityQueue 
{
    Input *values;
    int size;

} PriorityQueue;

void init_queue(PriorityQueue *queue);

bool is_empty(PriorityQueue *queue);

int compare_input(const void *a, const void *b);

void push(PriorityQueue *queue, Input input);

Input pop(PriorityQueue *queue);

void dump_ops(Input s);

