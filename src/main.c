#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <stdbool.h>

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

void gracefully_shutdown(int);
void enable_raw_mode();
void disable_raw_mode();
char* find_last_word(char*) ;
int get_index(char*, char);
void auto_complete(char*, int*);
void remove_null_terminators(char*, size_t);

void update_loop();
char* read_line();
char** tokenize(char*);
int execute(char**);
int launch_command(char** args);
int open_file_streams();
void close_file_streams();

int command_cd(char** args);
int command_help(char** args);
int command_exit(char** args);

int num_functions(char** functions[]);

int (*builtin_functions[])(char**) = {
    &command_cd,
    &command_help,
    &command_exit
};

char* command_one[]   = {"cd", "Change Directory."};
char* command_two[]   = {"help", "Show help about this shell."};
char* command_three[] = {"exit", "Exit the shell."};

char* suggestions[] = {"help", "hello", "history", "halt", "hack", NULL};

char** builtin_function_strings[] = {
    command_one,
    command_two,
    command_three
};

FILE* error_stream;
FILE* history_stream;
time_t current_time;
struct termios original_termios;

int main()
{
    int streams_opened = open_file_streams();
    if (!streams_opened)
        return EXIT_FAILURE;

    signal(SIGINT, gracefully_shutdown);
    signal(SIGKILL, gracefully_shutdown);
    signal(SIGTERM, gracefully_shutdown);

    enable_raw_mode();
        
    printf("Welcome to Neo's terminal\n");
    update_loop();

    close_file_streams();

    disable_raw_mode();
}

void update_loop()
{
    char* line;
    char** args;
    int status;

    do {
        printf("neo > ");
        line = read_line();

        current_time = time(NULL);
        if (current_time != (time_t)(-1))
            fprintf(history_stream, "%s - %s", line, asctime(gmtime(&current_time)));

        args = tokenize(line);
        status = execute(args);

        free(line);
        free(args);
    } 
    while (status);
}

char* read_line()
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
            auto_complete(buffer, &position);
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
            position = 0;
            return buffer;
        }
        else
            buffer[position++] = character;

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

char* find_last_word(char *buffer) {
    char *last_space = strrchr(buffer, ' '); // Find the last space
    return (last_space == NULL) ? buffer : last_space + 1;
}

void remove_null_terminators(char *arr, size_t length) {
    size_t j = 0; // Index to track the position for valid characters

    for (size_t i = 0; i < length; i++) {
        if (arr[i] != '\0') {
            arr[j++] = arr[i]; // Copy non-null characters to the next valid position
        }
    }

    // Nullify the remaining part of the array for safety
    while (j < length) {
        arr[j++] = '\0';
    }
}

void auto_complete(char* buffer, int* buffer_len)
{
    int len = strlen(buffer);
    char *last_word = find_last_word(buffer);
    size_t last_word_len = strlen(last_word);

    for (size_t i = 0; suggestions[i] != NULL; ++i)
        if (strncmp(last_word, suggestions[i], last_word_len) == 0)
        {
            // Replace last word with the suggestion
            strcpy(last_word, suggestions[i]);          // This asshole copies nullterminator which wasted my 5 hrs today
            break;                                      // Null terminator should not be copied in this case as printf stops printing as it encounters firse nullterminator
        }

    remove_null_terminators(buffer, strlen(buffer));      // This removes multiple null terminatos added by strcpy, which fixes printf's issue of not displaying the buffer properly

    // Update buffer length
    *buffer_len = strlen(buffer);

    // Log the auto completes to the stream
    char temp[500];
    sprintf(temp, "buffer: %s\n", buffer);
    fprintf(error_stream, temp);

    // Clear the terminal till cursor and print the new buffer
    printf("\r\033[Kneo > %s", buffer);
    fflush(stdout);
    return;
}

int get_index(char* arr, char c)
{
    for (const char *ptr = arr; *ptr != '\0'; ptr++) 
    {
        if (*ptr == c) 
            return ptr - arr; // Return the index by calculating the pointer difference
    }

    return -1; // Return -1 if the character is not found
}

char** tokenize(char* line)
{
    int buffSize = TOKEN_BUFFER_SIZE, position = 0;
    char** tokens = (char**)malloc(sizeof(char*) * buffSize);
    char* token = NULL;

    if (!tokens)
    {
        fprintf(stderr, "shell: allocation error in tokenize()");
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
                fprintf(stderr, "shell: reallocation error in tokenize()");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    tokens[position] = NULL;
    memset(line, 0, strlen(line) + 1);
    return tokens;
}

int execute(char** args)
{
    if (args[0] == NULL)
        return 1;

    for (size_t i = 0; i < num_functions(builtin_function_strings); ++i)
    {
        if (strcmp(args[0], builtin_function_strings[i][0]) == 0)
            return builtin_functions[i](args);
    }

    return launch_command(args);
}

int launch_command(char** args)
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

int num_functions(char** arr[])
{
    int count = 0;
    while (arr[count] != NULL)
        count++;

    return count;
}

/*
  Builtin function implementations.
*/

int command_cd(char** args)
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

int command_help(char** args)
{
    printf("Neo's custom shell\n");
    printf("Type program names followed by arguments, then hit enter.\n");
    printf("The commands are prebuilt in this shell:\n");

    for (size_t i = 0; i < num_functions(builtin_function_strings); ++i)
        printf("  %s -- %s\n", builtin_function_strings[i][0], builtin_function_strings[i][1]);

    printf("Use the \"man\" command for information on other programs.\n");

    return 1;
}

int command_exit(char** args)
{
    return 0;
}

void gracefully_shutdown(int signal)
{
    printf("Received signal: %i.\nShutting down...", signal);

    if (error_stream)
        fclose(error_stream);
    
    if (history_stream)
        fclose(history_stream);

    exit(EXIT_SUCCESS);
}

void enable_raw_mode()
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &original_termios);     // Get current terminal attributes
    raw = original_termios;
    //raw.c_lflag &= ~(ECHO | ICANON);            // disable canonical mode and echo
    raw.c_lflag &= ~ICANON;            // disable canonical mode and echo

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);   // Apply changes
}

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);  // Restore original settings
}

int open_file_streams()
{
    history_stream = fopen("shell_record.log", "a");
    if (!history_stream)
    {
        printf("Failed to open history file stream.\n");
        return 0;
    }

    error_stream = fopen("shell_errors.log", "a");
    if (!error_stream)
    {
        printf("Failed to open error file stream.\n");
        return 0;
    }
}

void close_file_streams()
{
    if (error_stream)
        fclose(error_stream);
    if (history_stream)
        fclose(history_stream);
}