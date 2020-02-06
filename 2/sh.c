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
    char *home;
    int success = -1;

    if (strcmp(input[1], "\0") == 0)
        return chdir(getenv("HOME"));

    return chdir(input[1]); 
}

/* Redirection Handler. Opens the file with applicable permissions. */
int redir(int direction, char file[]) {
    switch(direction) {
        case 1:
            close(0);
            return open(file, O_RDONLY);
        case 2:
            close(1);
            return open(file, O_WRONLY | O_CREAT, 0644);
        case 3:
            close(1);
            return open(file, O_WRONLY | O_APPEND, 0644);
        default:
            return 1;
    }
}

/* Redirection Specifier. Iterates through Argv, looks for indirection cases, and calls the Redirection Handler accordingly. */
int isRedir(char *argv[]) {
    char temp[16];
    for(int i = 0; argv[i]; strncpy(temp, argv[i++], 16))
        if (strcmp(temp, "<") == 0) { 
            strncpy(temp, argv[i], 16);
            return redir(1, temp);
        } else if (strcmp(temp, ">") == 0) { 
            strncpy(temp, argv[i], 16);
            return redir(2, temp);
        } else if (strcmp(temp, ">>") == 0) { 
            strncpy(temp, argv[i], 16);
            return redir(3, temp);
        }
    return -1;
}

/* Execution Handler. Handles and executes any command in the PATH, which for now is just /bin/ */
int exec(char *argv[]){
    char path[16] = "/bin/"; 

    strcat(path, argv[0]);  

    if (isRedir(argv) != -1) 
        argv[ARGC - 2] = NULL;

    if (execve(path, argv, NULL) == -1) 
        printf("%s\n", strerror(errno));
}

/* Pipe Handler: Combines data from piped commands. */
void concatPipe(char *head[], char *tail[]) {
    int pd[2], pid;
    pipe(pd); 

    pid = fork(); 

    if (pid) {        
        close(pd[0]); 

        close(1);     
        dup(pd[1]);   
        close(pd[1]); 
        exec(head);   
    } else {          
        close(pd[1]); 

        close(0);     
        dup(pd[0]);   
        close(pd[0]); 
        exec(tail);   
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
    int pid = fork(), status;

    if (pid > 0) {
        printf("Parent %d is now waiting for child %d to die.\n", getpid(), pid);
        
        pid = wait(&status);
        
        printf("Dead Child = %d, By = %04x\n", pid, status);
    } else if (pid == 0) { 
        char *head[16], *tail[16];

        if (isPipe(head, tail) == 1)
            concatPipe(head, tail);
        else
            exec(ARGV);
        
        exit(0);
    } else  
        printf("Unable to create process!\n");
}

/* Execution Specifier. Takes the information in Argv and calls commands accordingly. */
void execCMD(){
    generateArgv(ARGV); 

    if (strcmp(input[0], "exit") == 0) 
        exit(1);
    else if (strcmp(input[0], "cd") == 0) {
        if (cd() == 0) 
            printf("cd to %s was a Success.\n", (strcmp(input[1], "\0") == 0) ? "$HOME" : input[1]);
        else
            printf("cd to %s was a Failure.\n", input[1]);

        char cwd[256];
        getcwd(cwd, sizeof(cwd));
        printf("CWD ~> %s\n", cwd);
    } else
        forkC();
}

/* Main function. Handles the running of the program in a loop */
int main(int argc, char *argv[]) {
    char tempLine[128];

    while (1) {                     // No exit condition, as program exit is handle in execCMD()
        printf("$ ");               // PS1 Prompt
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