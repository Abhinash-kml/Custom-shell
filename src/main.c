#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

void UpdateLoop();
char* ReadLine();
char** SplitLine(char*);
int Execute();
int Launch(char** args);

int CommandCD(char** args);
int CommandHelp(char** args);
int CommandExit(char** args);

int NumFunctions(char* functions[]);

int (*builtinFunctions[])(char**) = {
    &CommandCD,
    &CommandHelp,
    &CommandExit
};

char* builtinFunctionStrings[] = {
    "cd",
    "help",
    "exit"
};

int main()
{
    printf("Hello World\n");
    UpdateLoop();
}

void UpdateLoop()
{
    char* line;
    char** args;
    int status;

    do {
        printf("neo > ");
        line = ReadLine();
        args = SplitLine(line);
        status = Execute(args);

        free(line);
        free(args);
    } 
    while (status);
}

char* ReadLine()
{
    int bufferSize = 246;
    int position = 0;
    char* buffer = (char*)malloc(sizeof(char) * bufferSize);
    int character;

    if (!buffer)
    {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        character = getchar();

        if (character == EOF || character == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
            buffer[position] = character;

        position++;

        if (position >= bufferSize)
        {
            bufferSize = bufferSize * 1.5;
            buffer = (char*)realloc(buffer, bufferSize);

            if (!buffer)
            {
                fprintf(stderr, "shell: reallocation failure");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char** SplitLine(char* line)
{
    int buffSize = TOKEN_BUFFER_SIZE, position = 0;
    char** tokens = (char**)malloc(sizeof(char*) * buffSize);
    char* token = NULL;

    if (!tokens)
    {
        fprintf(stderr, "shell: allocation error in SplitLine()");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOKEN_DELIMITERS);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= buffSize) 
        {
            buffSize = buffSize * 1.5;
            tokens = (char**)realloc(tokens, buffSize);

            if (!tokens)
            {
                fprintf(stderr, "shell: reallocation error in SplitLine()");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    tokens[position] = NULL;
    return tokens;
}

int Execute(char** args)
{
    if (args[0] == NULL)
        return 1;

    for (size_t i = 0; i < NumFunctions(builtinFunctionStrings); ++i)
    {
        if (strcmp(args[0], builtinFunctionStrings[i]) == 0)
            return builtinFunctions[i](args);
    }

    return Launch(args);
}

int Launch(char** args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        // Child process
        if (execvp(args[0], args) == -1)
            perror("shell");

        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        perror("shell");
    }
    else 
    {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } 
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int NumFunctions(char* functions[])
{
    return sizeof(functions) / sizeof(functions[0]);
}

/*
  Builtin function implementations.
*/

int CommandCD(char** args)
{
    if (args[0] == NULL)
        fprintf(stderr, "shell: argument expected for \"cd\"\n");
    else 
    {
        if (chdir(args[1]) != 0)
        perror("shell: directory change error");
    }

    return 1;
}

int CommandHelp(char** args)
{
    printf("Neo's custom shell\n");
    printf("Type program names followed by arguments, then hit enter.\n");
    printf("The commands are prebuilt in this shell:\n");

    for (size_t i = 0; i < NumFunctions(builtinFunctionStrings); ++i)
        printf("  %s\n", builtinFunctionStrings[i]);

    printf("Use the \"man\" command for information on other programs.\n");

    return 1;
}

int CommandExit(char** args)
{
    return 0;
}