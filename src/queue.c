#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../includes/server.h"
#include "../includes/queue.h"
#include "../includes/utils.h"

// void init_queue(PriorityQueue *queue)
// {
//     queue->values = xmalloc(sizeof(Input) * QSIZE);
//     queue->size = 0;
// }

// bool is_empty(PriorityQueue *queue)
// {
//     return (queue->size == 0);
// }

// int compare_input(const void *a, const void *b)
// {
//     Input *input_a = *(Input **)a;
//     Input *input_b = *(Input **)b;

//     if (input_a->priority < input_b->priority) return -1;
//     if (input_a->priority > input_b->priority) return 1;
//     else return 0;
// }

// void push(PriorityQueue *queue, Input *input)
// {
//     if (queue->size == QSIZE)
//     {
//         Input **queue_temp = realloc(queue->values, queue->size * sizeof(Input));

//         if (queue_temp == NULL) fprintf(stderr, "Failed to allocate memory.\n");
//         else queue->values = queue_temp;
//     }

//     queue->values[queue->size++] = input;
//     qsort(queue->values, queue->size, sizeof(Input*), &compare_input);
// }

// void pop(PriorityQueue *queue, Input *output)
// {
//     if (queue->size != 0)
//     {
//         output->from = xmalloc(sizeof(char) * strlen(queue->values[queue->size - 1]->from + 1));
//         strcpy(output->from, queue->values[queue->size - 1]->from);

//         output->to = xmalloc(sizeof(char) * strlen(queue->values[queue->size - 1]->to + 1));
//         strcpy(output->to, queue->values[queue->size - 1]->to);

//         output->proc_file = queue->values[queue->size - 1]->proc_file;
//         output->priority = queue->values[queue->size - 1]->priority;
//         output->op_len = queue->values[queue->size - 1]->op_len;
//         output->status = queue->values[queue->size - 1]->status;

//         output->operations = xmalloc(sizeof(char) * output->op_len * 16);

//         for (int i = 0; i < output->op_len; i++)
//         {
//             output->operations[i] = xmalloc(sizeof(char) * 16);
//             strcpy(output->operations[i], queue->values[queue->size - 1]->operations[i]);
//         }
         
//         queue->size--;
//         free(queue->values[queue->size]);
//         qsort(queue->values, queue->size, sizeof(Input*), &compare_input);
//     }


// }

/*
int main()
{
    char *from = malloc(sizeof(char) * 64);
    strcpy(from, "input/input.pssw");

    char *to = malloc(sizeof(char) * 64);
    strcpy(to, "output/output.hsd");

    Input *a = malloc(sizeof(Input));
    a->priority = 10; 
    a->from = from;
    a->to = to;
    a->op_len = 8;
    a->proc_file = true;
    a->status = QUEUED;
    a->operations = malloc(sizeof(char) * a->op_len * 16);
    a->operations[0] = strdup("nop");
    a->operations[1] = strdup("gcompress");
    a->operations[2] = strdup("nop");
    a->operations[3] = strdup("gcompresso");
    a->operations[4] = strdup("gcompress");
    a->operations[5] = strdup("nop");
    a->operations[6] = strdup("gcompresso");
    a->operations[7] = strdup("gdecompress");


    Input *b = malloc(sizeof(Input));
    b->priority = 6; 
    b->from = from;
    b->to = to;
    b->op_len = 3;
    b->proc_file = true;
    b->status = QUEUED;
    b->operations = malloc(sizeof(char) * b->op_len * 16);
    b->operations[0] = strdup("nop");
    b->operations[1] = strdup("gcompress");
    b->operations[2] = strdup("nop");

    Input *c = malloc(sizeof(Input));
    c->priority = 8; 
    c->from = from;
    c->to = to;
    c->op_len = 3;
    c->proc_file = true;
    c->status = QUEUED;
    c->operations = malloc(sizeof(char) * c->op_len * 16);
    c->operations[0] = strdup("nop");
    c->operations[1] = strdup("gcompress");
    c->operations[2] = strdup("nop");

    Input *d = malloc(sizeof(Input));
    d->priority = 3; 

    Input *e = malloc(sizeof(Input));
    e->priority = 10; 

    Input *f = malloc(sizeof(Input));
    f->priority = 4;

    Input *g = malloc(sizeof(Input));
    g->priority = 8; 

    Input *h = malloc(sizeof(Input));
    h->priority = 0;  

    PriorityQueue *q = malloc(sizeof(Input) * QSIZE + sizeof(int));
    init_queue(q);

    push(q, a);
    push(q, b);
    push(q, c);
    // push(q, d);
    // push(q, e);
    // push(q, f);
    // push(q, g);
    // push(q, h);

    printf("After push:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i]->priority);
    printf("\n\n");

    Input *result = malloc(sizeof(Input) * 4);
    pop(q, result);

    printf("After pop:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i]->priority);
    printf("\n\n");

    printf("Highest priority: %d\n", result->priority);
    printf("From: %s\n", result->from);
    printf("To: %s\n", result->to);
    printf("Op's: %d\n", result->op_len);
    printf("Status: %d\n", result->status);

    for (int i = 0; i < result->op_len; i++) printf("%s\n", result->operations[i]);
    printf("\n");

    Input *result2 = malloc(sizeof(Input) * 4);
    pop(q, result2);

    printf("After second pop:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i]->priority);
    printf("\n\n");

    printf("Highest priority: %d\n", result2->priority);
    printf("From: %s\n", result2->from);
    printf("To: %s\n", result2->to);
    printf("Op's: %d\n", result2->op_len);
    printf("Status: %d\n", result2->status);

    for (int i = 0; i < result2->op_len; i++) printf("%s\n", result2->operations[i]);
    printf("\n");
}
*/
