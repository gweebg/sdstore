#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main()
{
    char *in = "in.txt";
    char *out = "out.txt";

    /*
    nop: 0
    bcompress: 1
    bdecompress: 2
    gcompress: 3
    gdecompress: 4
    encrypt: 5
    decrypt: 6
    */

    // ./bcompress < in.txt | ./nop | ./gcompress | ./encrypt | ./nop > out.txt
    char *operations[5] = {"./bcompress", "./nop", "./gcompress", "./encrypt", "./nop"};
    // char *operations[4] = {"ls", "lolcat", "wc", "figlet"};
    char *revert[5] = {"nop", "decrypt", "gdecompress", "nop", "bdecompress"};

    // int num_commands = 5;
    int num_commands = 5;
    int num_pipes = num_commands - 1;

    int pipes[2 * num_pipes];
    pid_t pid;

    int in_fd = open(in, O_RDONLY, 0777);
    int out_fd = open(out, O_WRONLY | O_TRUNC | O_CREAT, 0777);

    if (in_fd < 0 || out_fd < 0)
    {
        perror("Could not open fd.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(pipes + (i * 2)) < 0)
        {
            perror("Could not open pipe.\n");
            exit(EXIT_FAILURE);
        }
    }


    int j = 0, command_count = 0;
    while (command_count < num_commands)
    {
        pid = fork();
        if (pid < 0)
        {
            perror("Failed to fork process.");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            if (command_count == 0)
            {
                if (dup2(in_fd, STDIN_FILENO) < 0)
                {
                    perror("Failed to dup2. <0>\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (command_count == num_commands - 1)
            {
                if (dup2(out_fd, STDOUT_FILENO) < 0)
                {
                    perror("Failed to dup2. <1>\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (command_count != num_commands - 1)
            {
                if (dup2(pipes[j + 1], 1) < 0)
                {
                    perror("Failed to dup2. <2>\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (j != 0)
            {
                if (dup2(pipes[j - 2], 0) < 0)
                {
                    perror("Failed to dup2 <3>.\n");
                    exit(EXIT_FAILURE);
                }
            }

            for (int u = 0; u < 2 * num_pipes; u++) close(pipes[u]);

            if (execlp(operations[command_count], operations[command_count], NULL) < 0)
            {
                perror("Could not execute.\n");
                exit(EXIT_FAILURE);
            }
        }

        command_count++;
        j+=2;
    }

    for (int i = 0; i < 2 * num_pipes; i++) close(pipes[i]);
    for (int i = 0; i < num_pipes + 1; i++) wait(NULL);

}