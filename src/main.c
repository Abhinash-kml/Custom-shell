#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

void GracefullShutdown(int);
void EnableRawMode();
void DisableRawMode();
char* findLastWord(char*) ;
void AutoComplete(char*, int*);

void UpdateLoop();
char* ReadLine();
char** SplitLine(char*);
int Execute();
int Launch(char** args);

int CommandCD(char** args);
int CommandHelp(char** args);
int CommandExit(char** args);

int NumFunctions(char** functions[]);

int (*builtinFunctions[])(char**) = {
    &CommandCD,
    &CommandHelp,
    &CommandExit
};

char* commandOne[]   = {"cd", "Change Directory."};
char* commandTwo[]   = {"help", "Show help about this shell."};
char* commandThree[] = {"exit", "Exit the shell."};

char* suggestions[] = {"help", "hello", "history", "halt", "hack", NULL};

char** builtinFunctionStrings[] = {
    commandOne,
    commandTwo,
    commandThree
};

FILE* errorStream;
FILE* recordStream;
time_t currentTime;
struct termios orig_termios;

int main()
{
    errorStream = fopen("shell_errors.log", "a");
    if (!errorStream)
    {
        printf("Error opening/creating error steam log");
        return EXIT_FAILURE;
    }

    recordStream = fopen("shell_record.log", "a");
    if (!recordStream)
    {
        printf("Error opening/creating record stream log");
        return EXIT_FAILURE;
    }

    signal(SIGINT, GracefullShutdown);
    signal(SIGKILL, GracefullShutdown);
    signal(SIGTERM, GracefullShutdown);

    EnableRawMode();
        
    printf("Welcome to Neo's terminal\n");
    UpdateLoop();

    if (errorStream)
        fclose(errorStream);
    if (recordStream)
        fclose(recordStream);

    DisableRawMode();
}

void UpdateLoop()
{
    char* line;
    char** args;
    int status;

    do {
        printf("neo > ");
        line = ReadLine();

        currentTime = time(NULL);
        if (currentTime != (time_t)(-1))
            fprintf(recordStream, "%s - %s", line, asctime(gmtime(&currentTime)));

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

        if (character == '\t')
            AutoComplete(buffer, &position);
        // else if (character == 10) // Enter key to execute command
        // { 
        //     buffer[position] = '\0';
        //     printf("\nExecuting: %s\n> ", buffer);
        //     position = 0;
        //     memset(buffer, 0, bufferSize);
        // } 
        else if (character == 127) // Backspace key
        { 
            if (position > 0) 
            {
                buffer[--position] = '\0';
                printf("\b \b", 3); // Erase last character
            }
        }
        else if (character == EOF || character == '\n')
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

char* findLastWord(char *buffer) {
    char *last_space = strrchr(buffer, ' '); // Find the last space
    return (last_space == NULL) ? buffer : last_space + 1;
}

void AutoComplete(char* buffer, int* buffer_len)
{
    int len = strlen(buffer);
    char *last_word = findLastWord(buffer);
    size_t last_word_len = strlen(last_word);
    //printf("\n");

    for (size_t i = 0; suggestions[i] != NULL; ++i)
        if (strncmp(last_word, suggestions[i], last_word_len) == 0)
        {
            size_t suggestion_len = strlen(suggestions[i]);

            // Replace last word with the suggestion
            strcpy(last_word, suggestions[i]);

            // Update buffer length
            *buffer_len = strlen(buffer);
        }

        // {
        //     strcpy(buffer, suggestions[i]);
        //     *buffer_len = strlen(suggestions[i]);
        // }

            
    //printf("\n%s", buffer); // Redisplay current input
    //printf("%s", buffer); // Redisplay current input
    printf("\r\033[Kneo > %s", buffer);
    //fflush(stdout);
    return;
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
        if (strcmp(args[0], builtinFunctionStrings[i][0]) == 0)
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

int NumFunctions(char** arr[])
{
    int count = 0;
    while (arr[count] != NULL)
        count++;

    return count;
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
        printf("  %s -- %s\n", builtinFunctionStrings[i][0], builtinFunctionStrings[i][1]);

    printf("Use the \"man\" command for information on other programs.\n");

    return 1;
}

int CommandExit(char** args)
{
    return 0;
}

void GracefullShutdown(int signal)
{
    printf("Received signal: %i.\nShutting down...", signal);

    if (errorStream)
        fclose(errorStream);
    
    if (recordStream)
        fclose(recordStream);

    exit(EXIT_SUCCESS);
}

void EnableRawMode()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);     // Get current terminal attributes
    raw = orig_termios;
    //raw.c_lflag &= ~(ECHO | ICANON);            // disable canonical mode and echo
    raw.c_lflag &= ~ICANON;            // disable canonical mode and echo

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);   // Apply changes
}

void DisableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);  // Restore original settings
}