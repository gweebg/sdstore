#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "../includes/utils.h"
#include "../includes/server.h"

/**
 * @brief Function that executes a job using system pipes.
 *
 * @param job Job to be executed.
 */
void execute(Input job)
{
    /*
    Exemplos de comandos:
    ./bcompress < in.txt | ./nop | ./gcompress | ./encrypt | ./nop > out.txt
    char *operations[4] = {"ls", "lolcat", "wc", "figlet"};
    */

    int num_commands = job.op_len;
    int num_pipes = num_commands - 1;

    int pipes[2 * num_pipes]; /* n pipes require n*2 channels */
    pid_t pid;

    /* Opening input and output file descriptors. */
    int in_fd = open(job.from, O_RDONLY, 0666);
    int out_fd = open(job.to, O_WRONLY | O_TRUNC | O_CREAT, 0666);

    if (in_fd < 0 || out_fd < 0)
    {
        print_error("Could not open file descriptor. (execute.c)\n");
        exit(OPEN_ERROR);
    }

    /* Opening requiered pipes. */
    for (int i = 0; i < num_pipes; i++)
    {
        if (pipe(pipes + (i * 2)) < 0)
        {
            print_error("Could not open pipe. (execute.c)\n");
            exit(PIPE_ERROR);
        }
    }

    /* Pipeline start. */
    int j = 0, command_count = 0;
    while (command_count < num_commands)
    {
        pid = fork();
        if (pid < 0)
        {
            print_error("Failed to fork process. (execute.c)\n");
            exit(FORK_ERROR);
        }

        if (pid == 0)
        {
            /* Se for o primeiro comando, precisamos de redirecionar o ficheiro de entrada 
            para o seu input. */
            if (command_count == 0)
            {
                if (dup2(in_fd, STDIN_FILENO) < 0)
                {
                    print_error("Failed to duplicate file descriptor [dup2].\n");
                    exit(DUP2_ERROR);
                }
            }

            /* Se for o último comando precisamos de redirecionar 
            o seu output para o ficheiro de saida. */
            if (command_count == num_commands - 1)
            {
                if (dup2(out_fd, STDOUT_FILENO) < 0)
                {
                    print_error("Failed to duplicate file descriptor [dup2].\n");
                    exit(DUP2_ERROR);
                }
            }

            if (command_count != num_commands - 1)
            {
                if (dup2(pipes[j + 1], STDOUT_FILENO) < 0)
                {
                    print_error("Failed to duplicate file descriptor [dup2].\n");
                    exit(DUP2_ERROR);
                }
            }

            if (j != 0)
            {
                if (dup2(pipes[j - 2], STDIN_FILENO) < 0)
                {
                    print_error("Failed to duplicate file descriptor [dup2].\n");
                    exit(DUP2_ERROR);
                }
            }

            for (int u = 0; u < 2 * num_pipes; u++) close(pipes[u]);

            if (execlp(job.operations[command_count], job.operations[command_count], NULL) < 0)
            {
                print_error("Failed to execute operations.\n");
                exit(EXEC_ERROR);
            }
        }

        command_count++;
        j+=2;
    }

    for (int i = 0; i < 2 * num_pipes; i++) close(pipes[i]);
    for (int i = 0; i < num_pipes + 1; i++) wait(NULL);
}