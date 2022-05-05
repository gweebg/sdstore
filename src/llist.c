#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../includes/llist.h"

void llist_push(struct Node **head_ref, PreProcessedInput new_data)
{
    struct Node *new_node = malloc(sizeof(struct Node));
  
    new_node->data = new_data;
    new_node->next = (*head_ref);  
    (*head_ref) = new_node;
}

void llist_delete(struct Node **head_ref, char *job_fifo)
{
    struct Node *temp = *head_ref, *prev;
 
    if (temp && (strcmp(temp->data.fifo, job_fifo) == 0)) 
    {
        *head_ref = temp->next;
        free(temp); 
        return;
    }

    while (temp && (strcmp(temp->data.fifo, job_fifo) != 0)) 
    {
        prev = temp;
        temp = temp->next;
    }
 
    if (!temp) return; 
    prev->next = temp->next;
 
    free(temp);
}