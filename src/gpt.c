#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAX_BUFFER 256
#define TAB_KEY 9
#define ENTER_KEY 10
#define PROMPT "neo > " // Define the shell prompt

const char *suggestions[] = {"help", "hello", "history", "halt", "hack", NULL};

// Function to find the last word in the buffer
char* findLastWord(char *buffer) {
    char *last_space = strrchr(buffer, ' '); // Find the last space
    return (last_space == NULL) ? buffer : last_space + 1;
}

// Function to autocomplete the last word
void AutoComplete(char *buffer, size_t *buffer_len) {
    char *last_word = findLastWord(buffer);
    size_t last_word_len = strlen(last_word);

    // Find the first matching suggestion
    for (size_t i = 0; suggestions[i] != NULL; ++i) {
        if (strncmp(last_word, suggestions[i], last_word_len) == 0) {
            size_t suggestion_len = strlen(suggestions[i]);

            // Replace last word with the suggestion
            strcpy(last_word, suggestions[i]);

            // Update buffer length
            *buffer_len = strlen(buffer);

            // Redisplay the completed input
            printf("\r\033[K%s%s", PROMPT, buffer); // Clear line and print prompt + input
            fflush(stdout);
            return;
        }
    }
}

// Raw mode functions
struct termios orig_termios;

void enableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Main function
int main() {
    char buffer[MAX_BUFFER] = {0};
    size_t buffer_len = 0;

    enableRawMode();
    printf("%s", PROMPT); // Print the initial prompt

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == -1) {
            perror("read");
            break;
        }

        if (c == TAB_KEY) { // Tab key for autocomplete
            AutoComplete(buffer, &buffer_len);
        } else if (c == ENTER_KEY) { // Enter key to execute command
            buffer[buffer_len] = '\0';
            printf("\nExecuting: %s\n%s", buffer, PROMPT); // Print result and prompt for the next line
            buffer_len = 0;
            memset(buffer, 0, MAX_BUFFER);
        } else if (c == 127) { // Backspace key
            if (buffer_len > 0) {
                buffer[--buffer_len] = '\0';
                printf("\b \b"); // Erase last character
            }
        } else {
            if (buffer_len < MAX_BUFFER - 1) {
                buffer[buffer_len++] = c;
                write(STDOUT_FILENO, &c, 1); // Echo character
            }
        }
    }

    disableRawMode();
    return 0;
}
