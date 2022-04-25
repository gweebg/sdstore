#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../includes/server.h"
#include "../includes/queue.h"
#include "../includes/utils.h"

/**
 * @brief Initializes a priority queue (sorted stack) with size as 0 and allocates memory for
 * it elements.
 * 
 * @param queue Given PriorityQueue object.
 */
void init_queue(PriorityQueue *queue)
{
    queue->values = xmalloc(sizeof(PreProcessedInput) * QSIZE);
    queue->size = 0;
}

/**
 * @brief Checks if a priority queue is empty or not.
 * 
 * @param queue Input queue.
 * @return true, if the queue is empty, false otherwise.
 */
bool is_empty(PriorityQueue *queue)
{
    if (queue->size == 0) return true;
    else return false;
}

/**
 * @brief Compares to Input elements. Used in sorting by the qsort function.
 *
 * @param a First element to compare.
 * @param b Seconde element to compare.
 * @return An integer value, -1 if a < b, 1 if a > b and 0 if a == b.
 */
int compare_input(const void *a, const void *b)
{
    PreProcessedInput *input_a = *(PreProcessedInput **)a;
    PreProcessedInput *input_b = *(PreProcessedInput **)b;

    if (input_a->priority < input_b->priority) return -1;
    if (input_a->priority > input_b->priority) return 1;
    else return 0;
}

/**
 * @brief Pushes an Input element onto the PriorityQueue.
 * 
 * @param queue Queue where to store the element.
 * @param input Element to store.
 */
void push(PriorityQueue *queue, PreProcessedInput input)
{
    if (queue->size == QSIZE)
    {
        PreProcessedInput *queue_temp = realloc(queue->values, queue->size * sizeof(PreProcessedInput));

        if (queue_temp == NULL) print_error("Failed to allocate memory.\n");
        else queue->values = queue_temp;
    }

    queue->values[queue->size] = input;
    queue->size = queue->size + 1;
    qsort(queue->values, queue->size, sizeof(PreProcessedInput), &compare_input);
}

/**
 * @brief Pops the element with highest priority of the queue.
 * 
 * @param queue Queue where to pop from.
 * @return The popped element.
 */
PreProcessedInput pop(PriorityQueue *queue)
{

    if (queue->size != QSIZE)
    {
        PreProcessedInput elem = queue->values[queue->size - 1];
        queue->size--;
        return elem;
    }
    else 
    {
        print_log("The queue is empty.\n");
        PreProcessedInput error = {.valid = -1};
        return error;
    }
}

/**
 * @brief Helper function to print out every operation of an Input element.
 * 
 * @param s Input struct.

void dump_ops(PreProcessedInput s)
{
    for (int i = 0; i < s.op_len; i++)
    {
        printf("%s ", s.operations[i]);
    }
    printf("\n");
}
*/

/*
int main()
{
    char *from = malloc(sizeof(char) * 64);
    strcpy(from, "input/input.pssw");

    char *to = malloc(sizeof(char) * 64);
    strcpy(to, "output/output.hsd");

    Input a;
    a.priority = 10; 
    a.from = from;
    a.to = to;
    a.op_len = 8;
    a.status = QUEUED;
    a.operations = malloc(sizeof(char) * a.op_len * 16);
    a.operations[0] = strdup("nop");
    a.operations[1] = strdup("gcompress");
    a.operations[2] = strdup("nop");
    a.operations[3] = strdup("gcompresso");
    a.operations[4] = strdup("gcompress");
    a.operations[5] = strdup("nop");
    a.operations[6] = strdup("gcompresso");
    a.operations[7] = strdup("gdecompress");


    Input b;
    b.priority = 6; 
    b.from = from;
    b.to = to;
    b.op_len = 3;
    b.status = QUEUED;
    b.operations = malloc(sizeof(char) * b.op_len * 16);
    b.operations[0] = strdup("nop");
    b.operations[1] = strdup("gcompress");
    b.operations[2] = strdup("nop");

    Input c;
    c.priority = 8; 
    c.from = from;
    c.to = to;
    c.op_len = 3;
    c.status = QUEUED;
    c.operations = malloc(sizeof(char) * c.op_len * 16);
    c.operations[0] = strdup("nop");
    c.operations[1] = strdup("gcompress");
    c.operations[2] = strdup("nop");

    PriorityQueue *q = malloc(sizeof(Input) * QSIZE + sizeof(int));
    init_queue(q);

    push(q, a);
    push(q, b);
    push(q, c);

    printf("After push:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i].priority);
    printf("\n\n");

    Input result = pop(q);

    printf("After pop:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i].priority);
    printf("\n\n");

    printf("Highest priority: %d\n", result.priority);
    printf("From: %s\n", result.from);
    printf("To: %s\n", result.to);
    printf("Op's: %d\n", result.op_len);
    printf("Status: %d\n", result.status);

    dump_ops(result);

    Input result2 = pop(q);

    printf("After second pop:\n");
    for (int i = 0; i < q->size; i++) printf("%d\n", q->values[i].priority);
    printf("\n\n");

    printf("Highest priority: %d\n", result2.priority);
    printf("From: %s\n", result2.from);
    printf("To: %s\n", result2.to);
    printf("Op's: %d\n", result2.op_len);
    printf("Status: %d\n", result2.status);

    dump_ops(result2);
}
*/
