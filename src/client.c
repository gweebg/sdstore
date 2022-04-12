/**
 * @file client.c
 * @author gweebg ; johnny_longo 
 * @brief Client side of the application. 
 * @version 0.1
 * @date 2022-04-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "includes/client.h"

/**
 * @brief Funtion that executes the whole client side as a programm.
 * 
 * @param argc Number or arguments.
 * @param argv Array containing command line arguments.
 * @return Error code (int).
 */
int main(int argc, char *argv[])
{
    /*
    Exemplo de input:
    ./sdstore proc-file <priority> samples/file-a outputs/file-a-output bcompress nop gcompress encrypt nop
    Menos de 6 argumentos raise erro.
    Also, o enunciado diz para assumir que o input é sempre válido.
    */

    if (argc < 6)
    {
        fprintf(stderr, "Not enought arguments.\nExpected at least 6 but got %d arguments, refer to the documentation for more information.\n", argc);
        return FORMAT_ERROR;
    }

    int operation_count = argc - 5;
    Input *input = malloc(sizeof(char) * 32 * operation_count +
                          sizeof(char) * strlen(argv[3]) + 1 + 
                          sizeof(char) * strlen(argv[4]) + 1 + 
                          sizeof(int) + 
                          sizeof(bool));

    if (!input)
    {
        fprintf(stderr, "Could not allocate memory (malloc failed).\n");
        return MALLOC_ERROR;
    }

    //TODO Povoar a struct.

    return 0;
}

