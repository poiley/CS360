
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int   ARGC = 0;
char *ARGV [16]; 
char  line [128]; 
char  input[16][16]; 

int cd(void) {
    char *home;
    int success = -1;

    if (strcmp(input[1], "\0") == 0)
        return chdir(getenv("HOME"));

    return chdir(input[1]); 
}

void generateArgv(char *destination[]) {
    int i = 0; 
    
    while (input[i][0] != 0) 
        destination[i] = input[i++];

    destination[i] = NULL; 
}

void cleanup() {
    ARGC = 0; 
    for (int i = 0; i < 16; i++) 
        memset(input[i], 0, strlen(input[i]));
}

int append(char *file) {
    close(1); 
    return open(file, O_WRONLY | O_APPEND, 0644);
}

int out(char *file) {
    close(1); 
    return open(file, O_WRONLY | O_CREAT, 0644);
}

int in(char *file) {
    close(0); 
    return open(file, O_RDONLY);
}

int redir(int direction, char filename[]) {
    switch(direction) {
        case 1:
            return in(filename);
        case 2:
            return out(filename);
        case 3:
            return append(filename);
        default:
            return 1;
    }
}

int isRedir(char *argv[]) {
    int i = 0;
    char temp[16];

    while (argv[i]) {
        strncpy(temp, argv[i], 16);

        if (strcmp(temp, "<") == 0) { 
            strncpy(temp, argv[++i], 16);
            redir(1, temp);
            return 1;
        } else if (strcmp(temp, ">") == 0) { 
            strncpy(temp, argv[++i], 16);
            redir(2, temp);
            return 2;
        } else if (strcmp(temp, ">>") == 0) { 
            strncpy(temp, argv[++i], 16);
            redir(3, temp);
            return 3;
        }
    }
    return -1;
}


int isPipe(char *head[], char *tail[]) {
    int i = 0;
    while (ARGV[i])
        if (strcmp(ARGV[i++], "|") == 0) { 
            int j = 0;
            while (ARGV[j])
                if (j < i) 
                    head[j] = ARGV[j];
                else if (j > i) 
                    tail[j - 3] = ARGV[j++];

            return 1;
        }

    return 0;
}

int exec(char *argv[]){
    char path[16] = "/bin/"; 

    strcat(path, argv[0]);  

    if (isRedir(argv) != -1) 
        argv[ARGC - 2] = NULL;

    if (execve(path, argv, NULL) == -1) 
        printf("%s\n", strerror(errno));
}

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

int forkC() {
    int pid = fork(), status;

    if (pid > 0) { 
        printf("Parent %d is not waiting for child %d to die.\n", getpid(), pid);
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

void execCMD(void){
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
        printf("cwd is: %s\n", cwd);
    } else
        forkC();
}

void tok(char src[]) {
    int i = 0;
    char *next = strtok(src, " "); 

    while (next != 0) { 
        strcpy(input[i++], next);
        next = strtok(NULL, " ");
        ARGC++;
    }
}

int main(int argc, char *argv[]){
    char tempLine[128];

    while (1) {
        printf("$ "); 
        fflush(stdout);

        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0; 
        strcpy(tempLine, line);
        tok(tempLine);  

        execCMD();
        cleanup();
    }

    return 0;
}