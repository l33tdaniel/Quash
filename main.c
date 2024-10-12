#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <dirent.h>  // Include for directory handling

#define CMD_LS      1
#define CMD_ECHO    2
#define CMD_HELP    3
#define CMD_EXIT    4
#define CMD_CAT     5
#define CMD_GREP    6
#define CMD_ENV_VAR 7
#define CMD_EXPORT  8
#define CMD_JOBS    9
#define CMD_KILL   10
#define CMD_CD     11

int main() {
    readInput();
    return 0;
}

int getCommandIndex(char *cmd) {
    if (strcmp(cmd, "ls")    == 0)    return CMD_LS;
    if (strcmp(cmd, "echo")  == 0)    return CMD_ECHO;
    if (strcmp(cmd, "help")  == 0)    return CMD_HELP;
    if (strcmp(cmd, "exit")  == 0)    return CMD_EXIT;
    if (strcmp(cmd, "cat")   == 0)    return CMD_CAT;
    if (strcmp(cmd, "grep")  == 0)    return CMD_GREP;
    if (strcmp(cmd, "export")== 0)    return CMD_EXPORT;
    if (strcmp(cmd, "jobs")  == 0)    return CMD_JOBS;
    if (strcmp(cmd, "kill")  == 0)    return CMD_KILL;
    if (strcmp(cmd, "cd")    == 0)    return CMD_CD;

    if (cmd[0] == '$') return CMD_ENV_VAR;  // Check for environment variable commands
    return 0; // Unknown command
}

bool outputRedirectionCheck(char *args[100]) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            return true;  // Output redirection found
        }
        i++;
    }
    return false;  // No output redirection found
}

bool appendCheck(char *args[100]) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], ">>") == 0) {
            return true;  // Append found
        }
        i++;
    }
    return false;  // No append found
}

bool inputRedirectionCheck(char *args[100]){
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            return true;  // Input redirection found
        }
        i++;
    }
    return false;  // No input redirection found
}

bool pipeCheck(char *args[100]) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            return true;  // Pipe found
        }
        i++;
    }
    return false;  // No pipe found
}

void parseThrough(char input[1024], char *args[100]){
    // Tokenize the input
    char *token = strtok(input, " ");
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;  // Null-terminate the argument array
    
    if (args[0] == NULL) {
        return;  // No command entered
    }
    
    bool hasPipe = pipeCheck(args);

    // Get the command index
    int cmdIndex = getCommandIndex(args[0]);

    switch (cmdIndex) {
        case CMD_LS: {
            DIR *dir;
            struct dirent *entry;

            // Open the current directory or specified directory
            if (args[1] == NULL) {
                dir = opendir(".");  // Use current directory
            } else {
                dir = opendir(args[1]);  // Use specified directory
            }

            if (dir == NULL) {
                perror("ls");  // Print error if the directory cannot be opened
                return;
            }

            // Read and print the directory entries
            while ((entry = readdir(dir)) != NULL) {
                printf("%s\n", entry->d_name);
            }

            closedir(dir);  // Close the directory
            break;
        }

        case CMD_ECHO: {
            // Echo all the arguments after "echo"
            for (int j = 1; args[j] != NULL; j++) {
                printf("%s ", args[j]);
            }
            printf("\n");  // Add a new line at the end
            break;
        }

        case CMD_HELP: {
            printf("User is asking for help- We hope you're enjoying the program!\n");
            break;
        }

        case CMD_CAT: {
            pid_t pid, wpid;
            int status;

            pid = fork();
            if (pid == 0) {
                // Child process
                if (execvp(args[0], args) == -1) {
                    perror("lsh");
                }
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                // Error forking
                perror("lsh");
            } else {
                // Parent process
                do {
                    wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            }
            break;
        }

        case CMD_GREP: {
            // action for grep
            break;
        }

        case CMD_EXIT: {
            printf("Goodbye!\n");
            exit(0);  // Return code 0 for normal exit
            break;
        }
        
        case CMD_ENV_VAR: {
            // Remove the '$' symbol and get the environment variable name
            char *env_var = getenv(args[0] + 1);  // Skip the '$' symbol
            if (env_var != NULL) {
                printf("Value of %s: %s\n", args[0], env_var);
            } else {
                printf("%s: No such environment variable\n", args[0]);
            }
            break;
        }

        case CMD_CD: {
            if (args[1] == NULL) {
                // If no argument is provided, change to the home directory
                if (chdir(getenv("HOME")) != 0) {
                    perror("cd");
                }
            } else {
                // Attempt to change to the specified directory
                if (chdir(args[1]) != 0) {
                    perror("cd");
                }
            }
            break;
        }
    }
}

void readInput(){
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
        parseThrough(input, args);
    }
}
