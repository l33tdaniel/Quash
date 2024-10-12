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
        
        // Print all the arguments
        printf("Arguments entered: \n");
        for (int j = 0; args[j] != NULL; j++) {
            printf("args[%d]: %s\n", j, args[j]);
        }
        if (strcmp(args[0], "ls") == 0) {
            printf("yes ls!\n");
        }
    }
    
    return 0;
}
