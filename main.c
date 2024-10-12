#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char input[1024];
    char *args[100];
    
    while (1) {
        printf("[QUASH]$ ");
        // Read user input
        if (fgets(input, 1024, stdin) == NULL) {
            break;  // Handle Ctrl+D (EOF)
        }
        
        // Remove newline character
        input[strcspn(input, "\n")] = 0;
        
        // Tokenize the input
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Null-terminate the argument array
        
        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "ls") == 0) {
            printf("you typed ls\n");
        }

        else if (strcmp(args[0], "echo") == 0) {
            // Echo all the arguments after "echo"
            for (int j = 1; args[j] != NULL; j++) {
                printf("%s ", args[j]);
            }
            printf("\n");  // Add a new line at the end
        }
        else if (strcmp(args[0], "help") == 0) {
            printf("User is asking for help- We hope you're enjoying the program!\n");
        }
        else if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            return 0;
        }
        
        else if (args[0][0] == '$') {
            // Remove the '$' symbol and get the environment variable name
            char *env_var = getenv(args[0] + 1);  // Skip the '$' symbol
            
            if (env_var != NULL) {
                printf("Value of %s: %s\n", args[0], env_var);
            } else {
                printf("%s: No such environment variable\n", args[0]);
            }
        }
    }
    
    return 0;
}
