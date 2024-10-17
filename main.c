#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>  // Include for directory handling
#include <fcntl.h>    // For file control options
#include <sys/types.h> // For pid_t and system call data types
#include <sys/stat.h>

//Included to define grep location
#define GREP_EXEC  "/bin/grep"

//Define max number of background tasks
#define MAX_JOBS 1024

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
#define CMD_PWD    12
#define CMD_TASK   99

struct background_task {
    pid_t pid;
    char command[256];
    char status[20];

};

struct background_task backTasks[MAX_JOBS];
int job_index = 0;

int getCommandIndex(char *cmd) {
    if (strcmp(cmd, "ls")    == 0)  return CMD_LS;
    if (strcmp(cmd, "echo")  == 0)  return CMD_ECHO;
    if (strcmp(cmd, "help")  == 0)  return CMD_HELP;
    if (strcmp(cmd, "exit")  == 0)  return CMD_EXIT;
    if (strcmp(cmd, "quit")  == 0)  return CMD_EXIT;
    if (strcmp(cmd, "cat")   == 0)  return CMD_CAT;
    if (strcmp(cmd, "grep")  == 0)  return CMD_GREP;
    if (strcmp(cmd, "export")== 0)  return CMD_EXPORT;
    if (strcmp(cmd, "jobs")  == 0)  return CMD_JOBS;
    if (strcmp(cmd, "kill")  == 0)  return CMD_KILL;
    if (strcmp(cmd, "cd")    == 0)  return CMD_CD;
    if (strcmp(cmd, "pwd")   == 0)  return CMD_PWD;

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

bool isBackgroundTask(char *args[100]) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            return true;  // Background task found
        }
        i++;
    }
    return false;  // No background sign found

}

void remove_back_process(pid_t pid){
    //Function that removes background processes
    for (int i = 0; i < job_index; ++i){
        //this finds the task in the list of background tasks with the matching pid 
        if (backTasks[i].pid == pid){
           // This then removes it from the list 
            for (int x = i; x < job_index - 1; ++x){
                backTasks[x] = backTasks[x + 1];
            }
            // This then decrements the count of jobs
            job_index--;
            break;
        }
    }
}
void parseThrough(char input[1024], char *args[100]){
    // Tokenize the input
    char *token = strtok(input, " ");
    /* char *backTasks[1024]; */
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;  // Null-terminate the argument array
    
    if (args[0] == NULL) {
        return;  // No command entered
    }
    
    bool hasPipe = pipeCheck(args); // |
    bool hasAppend = appendCheck(args); // >>
    bool hasInputRedirect = inputRedirectionCheck(args); // <
    bool hasOutputRedirect = outputRedirectionCheck(args); // >
    bool hasBackgroundTask= isBackgroundTask(args); // &
    // The order of operation is append + input + output before pipes
    // Handle output redirect > 
    if (hasOutputRedirect) {
        int fd;
        int saved_stdout;  // Save original stdout
        char *outputFile = NULL;

        // Find the output file name
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], ">") == 0) {
                outputFile = args[j + 1]; // The file after '>' is the output file
                args[j] = NULL; // Terminate the command before '>'
                break;
            }
        }

        if (outputFile) {
            saved_stdout = dup(STDOUT_FILENO); // Save the original stdout

            // Open the output file
            fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("Error opening file for output");
                return;
            }

            // Redirect standard output to the file
            dup2(fd, STDOUT_FILENO);  // Redirect stdout to file
            close(fd);                // Close the file descriptor since it’s duplicated

            // Fork a new process to execute the command
            pid_t pid = fork();
            if (pid < 0) {
                perror("Error forking process");
                return;  // Fork failed
            }

            if (pid == 0) {
                // In the child process: execute the command
                if (execvp(args[0], args) == -1) {
                    perror("Error executing command");
                    exit(EXIT_FAILURE); // Exit child process if exec fails
                }
            } else {
                // In the parent process: wait for the child to finish (optional)
                wait(NULL);  // You can comment this out if you don't want to wait
            }

            // Restore original stdout
            dup2(saved_stdout, STDOUT_FILENO);  // Restore original stdout
            close(saved_stdout);                // Close the saved stdout fd
        }
    }

    // Handle append (>>)
    if (hasAppend) {
        int fd;
        int saved_stdout;  // Save original stdout
        char *outputFile = NULL;

        // Find the output file name
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], ">>") == 0) {
                outputFile = args[j + 1]; // The file after '>' is the output file
                args[j] = NULL; // Terminate the command before '>'
                break;
            }
        }

        if (outputFile) {
            saved_stdout = dup(STDOUT_FILENO); // Save the original stdout

            // Open the output file
            fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("Error opening file for output");
                return;
            }

            // Redirect standard output to the file
            dup2(fd, STDOUT_FILENO);  // Redirect stdout to file
            close(fd);                // Close the file descriptor since it’s duplicated

            // Fork a new process to execute the command
            pid_t pid = fork();
            if (pid < 0) {
                perror("Error forking process");
                return;  // Fork failed
            }

            if (pid == 0) {
                // In the child process: execute the command
                if (execvp(args[0], args) == -1) {
                    perror("Error executing command");
                    exit(EXIT_FAILURE); // Exit child process if exec fails
                }
            } else {
                // In the parent process: wait for the child to finish (optional)
                wait(NULL);  // You can comment this out if you don't want to wait
            }

            // Restore original stdout
            dup2(saved_stdout, STDOUT_FILENO);  // Restore original stdout
            close(saved_stdout);                // Close the saved stdout fd
        }
    }
    // Handle background Task(&)
    if (hasBackgroundTask){
        int j = 0;
        while (args[j] != NULL) {
            //check if the background task symbol exists in string
            if (strcmp(args[j], "&") == 0) {
                args[j] = NULL; // Remove & from agrgs s otaht it doesn't error
                break;
            }
            j++;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process: execute the command
            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
                exit(EXIT_FAILURE);
            }
        } else if (pid > 0) {
            // Parent process: store background task information
            backTasks[job_index].pid = pid;
            strncpy(backTasks[job_index].command, args[0], sizeof(backTasks[job_index].command) - 1);
            backTasks[job_index].command[sizeof(backTasks[job_index].command) - 1] = '\0'; // Null-terminate
            strcpy(backTasks[job_index].status, "Running");
            printf("Background Task Started: [%d] %d %s %s\n", job_index + 1, backTasks[job_index].pid, backTasks[job_index].status, backTasks[job_index].command);
            job_index++;
            return;
        } else {
            perror("Error forking process");
        }
    }

    // Handle input redirection (<)
    if (hasInputRedirect) {

    }

    // Handle pipes (|)
    // returne
    if (hasPipe) {
        // Find the position of the pipe
        int pipePos = -1;
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "|") == 0) {
                pipePos = j;
                break;
            }
        }

        if (pipePos != -1) {
            args[pipePos] = NULL; // Split the command at the pipe

            // Prepare for piping
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("Error creating pipe");
                return;
            }

            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("Fork failed");
                return;
            }
            
            if (pid1 == 0) {
                // First child: set up pipe output to stdout
                close(pipefd[0]); // Close the read end of the pipe
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
                close(pipefd[1]); // Close write end of the pipe

                // Execute the first command (before the pipe)
                if (execvp(args[0], args) == -1) {
                    perror("Error executing first command in pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("Fork failed");
                return;
            }
            
            if (pid2 == 0) {
                // Second child: set up pipe input to stdin
                close(pipefd[1]); // Close the write end of the pipe
                dup2(pipefd[0], STDIN_FILENO); // Redirect stdin from pipe
                close(pipefd[0]); // Close read end of the pipe

                // Execute the second command (after the pipe)
                if (execvp(args[pipePos + 1], &args[pipePos + 1]) == -1) {
                    perror("Error executing second command in pipe");
                    exit(EXIT_FAILURE);
                }
            }

            // Parent process: close pipe and wait for children
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }


















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

        // Read and print the directory entries on one line, excluding . and ..
        while ((entry = readdir(dir)) != NULL) {
            // Skip the current directory (.) and parent directory (..)
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                printf("%s ", entry->d_name);  // Print each entry followed by a space
            }
        }
        
        printf("\n");  // Print a newline after listing all entries

        closedir(dir);  // Close the directory
        break;
    }

        case CMD_ECHO: {
            // Echo all the arguments after "echo"
            for (int j = 1; args[j] != NULL; j++) {
                if(*args[j] == '$'){
                    while (*args[j] != ' ') {
                        char *env_var = getenv(args[j] + 1);  // Skip the '$' symbol
                        if (env_var != NULL) {
                            printf("%s ", env_var);
                        } else {
                            printf("%s: No such environment variable ", args[0]);
                        }
                        break;
                    }
                }
                else {
                printf("%s ", args[j]);
                }
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
            pid_t pid;

            /* Make sure that they have enough parameters before executing */
            if (args[1] == NULL) {
                printf("Usage: grep [OPTION]... patterns [FILE]... \n");
                break;
            }            

            /* Fork to create a child process that runs eexec */
            pid = fork();
            if (pid == -1){
                //If the fork errors
                perror("fork");
                exit(0);
            }

            if (pid == 0){
                /* Inside of the child process */
                execvp("grep", args);
                perror("execvp");
            }else{
                /* Inside of the parent */
                int status;
                /* Here we wait for the child to finish */
                /* While child processes exist this value is greater than one */
                /* once it exits pid goes to -1 */
                while((pid = wait(&status)) > 0);
            }

            /* Break after the parent has waited fro the child or they have improper input */
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
        
        case CMD_PWD: {

            char buf[1024];
            if(getcwd(buf, sizeof(buf)) != NULL){
                printf("%s\n", buf);
            }else {
            perror("getcwd() error");
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

        case CMD_JOBS: {
            // Length of the tasks index
         for (int i = 0; i < job_index; i++) {
                if (backTasks[i].pid != 0) {  // Only print tasks with a valid PID
                    printf("[%d] %d %s %s\n", i + 1, backTasks[i].pid, backTasks[i].status, backTasks[i].command);
                }
            }
         break;

        }

        case CMD_EXPORT: {

        }

        case CMD_KILL: {
            if (args[1] == NULL){
                printf("Specify process to kill");
                break;
            }else{
                //This loops throguh the args and converts them to strings passing them to kill
                for (int i = 1; args[i] != NULL ; ++i) {
                    pid_t pid = atoi(args[i]);
                
                    if (kill(pid, SIGKILL) == -1){
                        perror("Error killing process");

                    }else{
                        printf("Process killed \n");
                        //call on removing backround processes
                        remove_back_process(pid);

                    }
                }
            }
         break;
        }
        case CMD_TASK: {
            if (job_index > 0){
            printf("hi");
            }
                break;
       }

        case 0: {
            printf("Unknown command case\n");
        }
    }
}

/* we could declare this in a header file and move main to the top of this file if necessary */
void readInput(){
    char input[1024];
    char *args[100];

    while (1) {
        printf("[QUASH]$ ");
        // Read user input
        if (fgets(input, 1024, stdin) == NULL) {
            break;  // Handle Ctrl+D (EOF)
        }
        input[strcspn(input, "\n")] = 0; // Remove newline character
        parseThrough(input, args);
        /*****
        
         * How will we handle things such as multi string inputs to echo for example?


        More test examples: 
        sort < unsorted.txt > sorted.txt
        echo "Appending another line." >> output.txt
        ps aux | grep "bash"
        cat input.txt | grep "keyword" > filtered.txt

        *****/
    }
}

/* Main is placed at the bottom to ensure that readInput is recoginzed and not assigned an implicit type and return an Int */
int main() {
    readInput();
    return 0;
}
