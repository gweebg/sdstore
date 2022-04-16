/**
 * @file server.c
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

#include "../includes/server.h"

/**
 * @brief A better version of malloc that removes the work of checking for error.
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
 * @brief Function that halts the execution because of an write error.
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
Input createInput(char *string){
    int bytesRead, i = 1;
    Input r;
    r.proc_file = false;
    char *buf = strtok(string, " ");
    r.priority = atoi(buf);
    buf = strtok(NULL, " ");
    r.from = strdup(buf);
    buf = strtok(NULL, " ");
    r.to = strdup(buf);
    r.operations = malloc(sizeof(char*)*i);
    for(buf = strtok(NULL, " "); buf != NULL; i++){
        if(strcmp(buf, "encrypt"))
            r.proc_file = true;

        r.operations = realloc(r.operations, sizeof(char*)*i);
        r.operations[i-1] = strdup(buf);
        buf = strtok(NULL, " ");
    }

    return r;
}

int main()
{
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

    write(STDOUT_FILENO, "[!] Server is online!\n" , 23);
    write(STDOUT_FILENO, "[*] Listening for data...\n", 27);

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
        if (read(client_to_server, arguments, BUFSIZ) < 0) raise_read_error();
        else if (strcmp(arguments, "") != 0) printf("[*] %s\n\n", arguments);

        /* Reset buffer */
        memset(arguments, 0, BUFSIZ);
    }

    close(client_to_server);
    close(server_to_client);

    /*
    unlink(cts_fifo);
    unlink(stc_fifo);
    */

    return 0;
}