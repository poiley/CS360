// Lab 2 - Benjamin Poile

#include <stdio.h>      // printf(), fflush(), fgets(), stdout, stdin
#include <stdlib.h>     // getenv(), exit()
#include <string.h>     // memset(), strlen(), strtok(), strerror(), strcmp(), strcat(), strncpy()
#include <unistd.h>     // close(), pipe(), dup(), getpid(), getcwd()
#include <sys/wait.h>   // wait()
#include <errno.h>      // errno
#include <fcntl.h>      // open(), O_RDONLY, O_WRONLY, O_CREAT, O_APPEND

int   ARGC = 0;
char *ARGV [16]; 
char  line [128]; 
char  input[16][16]; 

/* Populates and returns an array of strings containing input from the user. */
void generateArgv(char *argv[]) {
    int i;
    for(i = 0; input[i][0] != 0; i++)
        argv[i] = input[i];
    argv[i] = NULL; 
}

/* Wipes the stored input from the user, so sh can be ready for the next command */
void cleanup() {
    for (int i = 0; i < 16; i++) 
        memset(input[i], 0, strlen(input[i]));
    ARGC = 0; 
}

/* Tokenize (split) input from the user into parsable objects. */
void tok(char src[]) {
    char *next = strtok(src, " "); 

    for(int i = 0; next != 0; i++, ARGC++) {
        strcpy(input[i], next);
        next = strtok(NULL, " ");
    }
}

/* Change the directory that the program is pointing to. By deault, Home. */
int cd() {
    if (strcmp(input[1], "\0") == 0)    // If no directory specified, go home.
        return chdir(getenv("HOME"));
    return chdir(input[1]);             // Go to directory specified.
}

/* Redirection Handler. Opens the file with applicable permissions. */
int redir(int o, char file[]) {
    switch(o) {                         // int o determines file open configuration. 
        case 1:                         // case 1: <    Open file as Read Only.
            close(0);
            return open(file, O_RDONLY);//open file as read only
        case 2:                         // case 2: >    Open or create file as Write Only.
            close(1);
            return open(file, O_WRONLY | O_CREAT, 0644);
        case 3:                         // case 3: >>   Open file as Write Only and append.
            close(1);
            return open(file, O_WRONLY | O_APPEND, 0644);    
    }
    return 1; // Default: Edge case, no o
}

/* Redirection Specifier. Iterates through Argv, looks for indirection cases, and calls the Redirection Handler accordingly. */
int isRedir(char *argv[]) {
    char temp[16];
    for(int i = 0; argv[i]; strncpy(temp, argv[i++], 16)) // If <, >, or >>, copy the argument string each time and redirect.
        if (strcmp(temp, "<") == 0) {           // case 1: <    Copy string, redirect.
            strncpy(temp, argv[i], 16); 
            return redir(1, temp);
        } else if (strcmp(temp, ">") == 0) {    // case 2: >    Copy string, redirect. 
            strncpy(temp, argv[i], 16);     
            return redir(2, temp);
        } else if (strcmp(temp, ">>") == 0) {   // case 3: >>   Copy string, redirect.
            strncpy(temp, argv[i], 16);
            return redir(3, temp);
        }
    return -1;                                  // case default. return -1.
}

/* Execution Handler. Handles and executes any command in the PATH, which for now is just /bin/ */
int exec(char *argv[]){
    char path[16] = "/bin/";            // PATH: Eventually will be an environment variable, but for now it just refers to /bin/
    strcat(path, argv[0]);  

    if (isRedir(argv) != -1)            // If redirect, set data.
        argv[ARGC - 2] = NULL;

    if (execve(path, argv, NULL) == -1) // Execute and handle errors.
        printf("%s\n", strerror(errno));
}

/* Pipe Handler: Pipes commands and combines data. */
void concatPipe(char *head[], char *tail[]) {
    int pd[2], pid;
    pipe(pd);           // Determines and routes the input/output for piped processes.

    pid = fork();       // Fork.

    if (pid) {
        close(pd[0]);   // Close the input stream.

        close(1);     
        dup(pd[1]);   
        close(pd[1]); 
        exec(head);     // Execute.
    } else {          
        close(pd[1]);   // Close the input stream.

        close(0);     
        dup(pd[0]);   
        close(pd[0]); 
        exec(tail);     // Execute.
    }
}

/* Pipe Specifier. Iterates through Argv, looks for pipes, and calls the Pipe Handler accordingly. */
int isPipe(char *head[], char *tail[]) {
    for(int i = 0; ARGV[i]; i++)
        if (strcmp(ARGV[i], "|") == 0) {
            for(int j = 0; ARGV[j]; j++)
                if (j < i)
                    head[j] = ARGV[j];
                else if (j > i)
                    tail[j - 3] = ARGV[j];
            return 1;
        }
    return 0;
}

/* Function to handle the forking of child processes. */
int forkC() {
    int pid = fork(), status; // Fork child process, and create status int.

    if (pid > 0) {
        printf("[LOG] Parent %d is now waiting for child %d to die.\n", getpid(), pid);
        
        pid = wait(&status); // Wait for child process.
        
        printf("[LOG] Dead Child = %d, By = %04x\n", pid, status);
    } else if (pid == 0) { 
        char *head[16], *tail[16];      // Create Head and Tail.

        if (isPipe(head, tail) == 1)    // Check for pipes in Head and Tail.
            concatPipe(head, tail);     // Concat Pipe data.
        else
            exec(ARGV);
        
        exit(0);                        // Exit.
    } else  
        printf("[LOG] Unable to create process!\n");    // Report Error.
}

/* Execution Specifier. Takes the information in Argv and calls commands accordingly. */
void execCMD(){
    generateArgv(ARGV);                      // Populate global Argv.

    if (strcmp(input[0], "exit") == 0)      // If the has entered the exit command...
        exit(1);                            // and stop the program.
    else if (strcmp(input[0], "cd") == 0) { // Otherwise, check if the user has entered cd, which is a special case.
        if (cd() == 0)                      // By default: cd to Home directory if cd directory isn't specified.
            printf("[LOG] cd to %s was a Success.\n", (strcmp(input[1], "\0") == 0) ? "$HOME" : input[1]);
        else
            printf("[LOG] cd to %s was a Failure.\n", input[1]); // Else, output the current working directory.

        char cwd[256];
        getcwd(cwd, sizeof(cwd));
        printf("[LOG] CWD ~> %s\n", cwd);
    } else
        forkC();                            // Execute the child command
}

/* EXTRA: Function that gets the CWD and returns it as a *char */
char *cwd() {
    char buf[256];                      // Initialize buffer character array.
    return getcwd(buf, sizeof(buf));    // Get current working directory and return it.
}

/* Main function. Handles the running of the program in a loop */
int main(int argc, char *argv[]) {
    char tempLine[128];             // Temporary string variable initialization.

    while (1) {                     // No exit condition, as program exit is handle in execCMD()
        printf("%s$ ", cwd());      // Line Prompt
        fflush(stdout);             // Flush

        fgets(line, 128, stdin);    // Read user input, write to line
        line[strlen(line) - 1] = 0; // Format
        strcpy(tempLine, line);     // Copy to temp value
        tok(tempLine);              // Tokenize

        execCMD();                  // Run user input
        cleanup();                  // Clean up before next run.
    }

    return 0;
}