#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../includes/llist.h"

void llist_push(struct Node **head_ref, char *new_data)
{
    struct Node *new_node = malloc(sizeof(struct Node));
  
    new_node->data = strdup(new_data);
    new_node->next = (*head_ref);  
    (*head_ref) = new_node;
}

void llist_delete(struct Node **head_ref, char *job_fifo)
{
    struct Node *temp = *head_ref, *prev = NULL;
 
    char *head_fifo = strtok(strdup(temp->data), " ");
    if (temp && (strcmp(head_fifo, job_fifo) == 0)) 
    {
        *head_ref = temp->next;
        free(temp); 
        return;
    }

    while (temp) 
    {
        char *dupped_string = strdup(temp->data);
        if (strcmp(strtok(dupped_string, " "), job_fifo) == 0) break;
        
        prev = temp;
        temp = temp->next;

        free(dupped_string);
    }
 
    if (!temp) return; 
    prev->next = temp->next;
 
    free(temp);
}