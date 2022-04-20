#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../includes/utils.h"

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
 * @brief Returns the number of operations on an input string.
 * 
 * @param string Input string.
 * @return Number of operations (int).
 */
int get_commands_len(char *string)
{
    char *tok = strtok(string, " ");

    int i = 0;
    while(tok != NULL) 
    {
        i++;
        tok = strtok(NULL, " \n");
    }

    return i - 4;
}

ssize_t read_line(int fd, char* line, size_t size)
{
    ssize_t bytes_read = read(fd, line, size);
    if (!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;
    if (bytes_read < line_length) line_length = bytes_read;
    line[line_length] = 0;
    
    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}

Configuration generate_config(char *path)
{
    Configuration result;
    int conf_file = open(path, O_RDONLY);

    if (conf_file == -1)
    {
        write(STDERR_FILENO, "[!] Could not read configuration file.\n", 40);
        exit(OPEN_ERROR);
    }

    char *current_line = xmalloc(sizeof(char) * 64);
    char *rest = xmalloc(sizeof(char) * 64);
    for (int i = 0; i < 7; i++)
    {
        read_line(conf_file, current_line, 64);
        char *operation = strtok_r(current_line, " ", &rest);
        int max = atoi(rest);

        if (strcmp(operation, "nop") == 0) result.nop = max;
        else if (strcmp(operation, "bcompress") == 0) result.bcompress = max;
        else if (strcmp(operation, "bdecompress") == 0) result.bdecompress = max;
        else if (strcmp(operation, "gcompress") == 0) result.gcompress = max;
        else if (strcmp(operation, "gdecompress") == 0) result.gdecompress = max;
        else if (strcmp(operation, "encrypt") == 0) result.encrypt = max;
        else if (strcmp(operation, "decrypt") == 0) result.decrypt = max;
        else
        {
            write(STDERR_FILENO, "[!] Invalid configuration file.\n", 33);
            exit(FORMAT_ERROR);
        }
    }

    free(current_line);
    close(conf_file);

    return result;
}