#pragma once

#include "server.h"

struct Node
{
    char *data;
    struct Node *next;
};

void llist_push(struct Node **head_ref, char *new_data);

void llist_delete(struct Node **head_ref, char *job_fifo);