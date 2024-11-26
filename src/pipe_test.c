#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

void execute_commands(char* command_one[], char* command_two[])
{
    int pipe_fd[2];    // 0 - Read end, 1 - Write end
    pid_t pid_one, pid_two;

    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Create first child process by calling fork()
    pid_one = fork();
    if (pid_one == 0) // Child process created 
    {
        dup2(pipe_fd[1], STDOUT_FILENO);        // Redirect stdout of this process to pipe's stdout i.e. pipe_fd[1] = 1
        close(pipe_fd[1]);                      // Close stdout of this process, as its output is sent to pipe's stdout i.e pipe_fd[1] = 1
        close(pipe_fd[0]);                      // Close stdin of this process, as its not reading anything from stdin 
        execvp(command_one[0], command_one);    // Execute command_one
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // Create second child process by calling fork()
    pid_two = fork();
    if (pid_two == 0)
    {
        dup2(pipe_fd[0], STDIN_FILENO);         // Redirect stdin of this process to pipe's stdin i.e. pipe_fd[0] = 0
        close(pipe_fd[1]);                      // Close stdout of this process as its not outputting anything to stdout
        close(pipe_fd[0]);                      // Close stdin of this process, as stdin is redirected to pipe's stdin i.e. pipe_fd[0] = 0
        execvp(command_two[0], command_two);    // Execute command_two
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[0]);  // Close pipe's stdin i.e. read end in parent process
    close(pipe_fd[1]);  // Close pipe's stdout i.e. write end in parent process

    // Wait for child processes to complete
    waitpid(pid_one, NULL, 0);  // Wait for child process one to complete
    waitpid(pid_two, NULL, 0);  // Wait for child process two to complete
}

int main(int argc, char** argv)
{
    printf("pipe test\n");
    char* cmd1[] = {"ls", NULL};
    char* cmd2[] = {"grep", ".log", NULL};

    execute_commands(cmd1, cmd2);

    getchar();
}