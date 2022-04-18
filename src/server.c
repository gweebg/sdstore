/**
 * @file server->c
 * @author gweebg ; johnny_longo 
 * @brief Server side of the application. 
 * @version 0.1
 * @date 2022-04-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "../includes/server.h"

/**
 * @brief A better version of malloc that removes the work of checking for error->
 * 
 * @param size Number of bytes to allocate.
 * @return Address of the new allocated memory block.
 */
void *xmalloc(size_t size)
{
    void *result = malloc(size);
    if (!result)
    {
        fprintf(stderr, "Failed to allocate memory (malloc error).\n");
        return NULL;
    }
    return result;
}

/**
 * @brief Function that halts the execution because of an write error->
 */
void raise_read_error()
{
    fprintf(stderr, "[!] Could not read from FIFO.\n");
    exit(READ_ERROR);
}

/**
* @brief Builds a struct from a String.
*
* @param string string with the info for the struct.
* @return Built struct input.
*/
void create_input(char *string, Input *r){
    int i = 1;
    r->proc_file = false;
    char *buf = strtok(string, " ");
    r->priority = atoi(buf);
    buf = strtok(NULL, " ");
    r->from = strdup(buf);
    buf = strtok(NULL, " ");
    r->to = strdup(buf);
    r->operations = malloc(sizeof(char*)*i);
    for(buf = strtok(NULL, " "); buf != NULL; i++){
        if(strcmp(buf, "encrypt"))
            r->proc_file = true;

        r->operations = realloc(r->operations, sizeof(char*)*i);
        r->operations[i-1] = strdup(buf);
        buf = strtok(NULL, " ");
    }
}

int compare_priorities(const void *a, const void *b){
    const Input *i1 = (const Input *)a;
    const Input *i2 = (const Input *)b;
    if(i1->priority < i2->priority)
        return 1;
    if(i1->priority > i2->priority)
        return -1;
    return 0;
}

Input* create_queue(Input *arr, Input i){
    arr = malloc(sizeof(Input));
    arr[0] = i;
    return arr;
}

int insert_elem(Input *arr, int size, Input i){
    if(size == 0){
        arr = create_queue(arr, i);
        return 0;
    }
    arr = realloc(arr, size+1);
    if(arr == NULL)
        return -1;

    arr[size] = i;
    qsort(arr, size+1, sizeof(Input), compare_priorities);
    return 1;
}

int cat_queues(Input *arr1, int size1, Input *arr2, int size2){
    arr1 = realloc(arr1, sizeof(Input) * (size1+size2));
    if(arr1 == NULL || size1*size2 <= 0)
        return -1;
    int i = size1, j = 0;
    while(i < size1-1+size2 && j < size2){
        arr1[i] = arr2[j];
        i++;
        j++;
    }
    qsort(arr1, size1+size2, sizeof(Input), compare_priorities);
    free(arr2);
    return 1;
}

int pop_queue(Input *arr, int size, Input *r){
    *r = arr[0];
    memmove(arr, &arr[1], size-1);
    arr = realloc(arr, sizeof(Input) * size-1);
    if(arr == NULL)
        return -1;
    return 1;
}

int main()
{
    /*
    fd[0] - read
    fd[1] - write
    */

    int input_com[2];

    if (pipe(input_com) == -1)
    {
        printf("Something went wrong while creating the pipe.\n");
        return PIPE_ERROR;
    }

    pid_t pid_main = fork();
    if (pid_main < 0)
    {
        printf("Something went wrong while creating a new process.\n");
        return FORK_ERROR;
    }

    if (pid_main == 0)
    {
        /* 
        Child Process
        Listener dos pedidos enviados pelo servidor e tambem, atraves de um pipe, envia o pedido
        para o processo pai para que este seja armazenado de acordo com a prioridade numa priority
        queue. 
        */

        close(input_com[0]);

        /* cts <=> client_to_server */
        int client_to_server;
        const char *cts_fifo = "com/cts";
        mkfifo(cts_fifo, 0666);

        /* stc <=> server_to_client */
        int server_to_client;
        const char *stc_fifo = "com/stc";
        mkfifo(stc_fifo, 0666);

        client_to_server = open(cts_fifo, O_RDONLY);
        server_to_client = open(stc_fifo, O_WRONLY);

        // write(STDOUT_FILENO, "[!] Server is online!\n" , 23);
        // write(STDOUT_FILENO, "[*] Listening for data...\n", 27);

        char arguments[BUFSIZ];
        while(true)
        {
            /* 
            Ler o valor dos argumentos enviados pelo client (str) (primeiro o tamanho e depois a string)
            int args_len, read_bytes;
            if ((read_bytes = read(client_to_server, &args_len, sizeof(int))) < 0) raise_read_error();
            else if (read_bytes != 0) printf("[>] %d\n", args_len);
            */

            // TODO Para tornar mais eficiente convém mandar primeiro o tamanho da string.
            if (read(client_to_server, arguments, BUFSIZ) < 0) 
            {
                fprintf(stderr, "[!] Could not read from FIFO.\n");
                _exit(READ_ERROR);
            }
            else if (strcmp(arguments, "") != 0) 
            {
                printf("[sent>] %s\n\n", arguments);

                int input_length = strlen(arguments) + 1; 

                if (write(input_com[1], &input_length, sizeof(int)) < 0)
                {
                    printf("Something went wrong while writing to pipe.\n");
                    _exit(WRITE_ERROR);
                }

                if (write(input_com[1], arguments, sizeof(char) * input_length) < 0)
                {
                    printf("Something went wrong while writing to pipe.\n");
                   _exit(WRITE_ERROR);
                }
            }

            /* Reset buffer */
            memset(arguments, 0, BUFSIZ);
        }

        close(client_to_server);
        close(server_to_client);
        close(input_com[1]);

        /*
        unlink(cts_fifo);
        unlink(stc_fifo);
        */

        _exit(EXIT_SUCCESS);
    }
    else
    {
        /* 
        Parent Process 
        Recebe os pedidos pelo pipe do processo filho faz o parsing e armazena numa priority queue.
        */
       
        char input_string[BUFSIZ];
        while (true)
        {
            close(input_com[1]);
            int size;

            if (read(input_com[0], &size, sizeof(int)) < 0)
            {
                printf("Something went wrong while reading from pipe.\n");
                _exit(READ_ERROR);
            }

            if (read(input_com[0], input_string, sizeof(char) * size) < 0)
            {
                printf("Something went wrong while reading from pipe.\n");
                _exit(READ_ERROR);
            }

            printf("[received<] %s\n", input_string);

            /* Reset buffer */
            memset(input_string, 0, BUFSIZ);

        }
        
        close(input_com[0]);
        _exit(EXIT_SUCCESS);
    }

    wait(NULL);
    return 0;
}