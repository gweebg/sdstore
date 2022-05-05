#pragma once

#include "server.h"

struct Node
{
    PreProcessedInput data;
    struct Node *next;
};

void llist_push(struct Node **head_ref, PreProcessedInput new_data);
void llist_delete(struct Node **head_ref, char *job_fifo);