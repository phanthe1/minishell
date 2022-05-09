#define _GNU_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

//Constants
#define MAX_LENGTH 2048 //command line max length characters
#define MAX_ARGS 512 //command line max length arguments

//Function Declarations
void getInput();
void clearInput();
void clearArgs();
void execProc();
void fileRedirection();

//Globals 
char command[MAX_LENGTH];
char *argList[MAX_ARGS];
char outputFileName[256];
char inputFileName[256];
size_t curSize = 0;
int argSize = 0;
int ch;
int targetFD;
bool redirection = false;
bool background = false;

// Main Function
int main(){

    while(1){
        getInput(); //Function call to get user input
        // printf("command: %s", command);
        // char *token = strtok(command, " ");
        const char delimiters[] = " " " \n"; //List of delimiters
        argList[argSize] = strtok(command, delimiters);
        while(argList[argSize] != NULL){
            // printf("argSize: %d Token: %s\n", argSize, argList[argSize]);
            argList[++argSize] = strtok(NULL, delimiters);
        }

        if(strcmp(argList[argSize - 1], "&") == 0){
            printf("ENTER HERE BECAUSE &\n");
            background = true; //set background process to true (yes, it is background process);
            argList[argSize - 1] = NULL; //Set the index that contains & to null so that it doesn't run through execution function.
            for(int i = 0; i < argSize; i++){
                printf("argList[%d]: %s\n", i, argList[i]);
            }
        }

        //Blank or comment(#)
        if(strchr(command, '#') || command[0] == '\n' || command[0] == ' '){
            printf("THIS IS A BLANK OR COMMENT\n");
            curSize = 0;
            continue;
        }
        //Exit command
        else if(strstr(command, "exit")){
            printf("THIS IS AN EXIT COMMAND\n");
            break;
        }
        //Change directory to Home if only "cd"
        else if(strstr(command, "cd")){
            printf("CHANGING DIRECTORY\n");
            // printf("Argsize print here: %d \n", argSize);
            //If only cd
            if(argSize < 2){
                chdir(getenv("HOME"));
            }
            //If cd and a directory
            else{
                // printf("ArgList string: %s", argList[1]);
                chdir(argList[1]);
            }
        }
        //Status
        else if(strstr(command, "status")){
            printf("THIS IS THE STATUS COMMAND\n");
        }
        else{
            printf("ENTER HERE FOR EXECUTION\n");
            execProc();
        }
        clearInput(); //Clear command
        clearArgs(); //Clear token arguments
    }
}

//WRITE SOURCE HERE
//Execution process for other commands
void execProc(){
    // printf("Execution Process here\n");

    pid_t pidSpawn = fork();
    int childStatus;
    // int childPid;

    // printf("Pid value: %d\n", pidSpawn);
    
    switch(pidSpawn){

        int status;
        printf("Status: %d", status);

        //Error if unable to fork
        case -1:
            perror("ERROR in creating child process");
            exit(1);
            break;
        //Child Process
        case 0:
            fileRedirection();
            // printf("I am the child. My pid is %d\n", getpid());
            execvp(argList[0], argList); //Excecute commands 
            printf("DIDN'T WORK: %s \n", argList[0]);
            fflush(NULL);
            perror(" ");
            exit(1);
        //Parent Process
        default:
            // printf("I am the parent. My pid  = %d\n", getpid());
            waitpid(pidSpawn, &childStatus, 0); //Block hits parent until the specified child process terminates.
            // printf("Parent's waiting is done as the child with pid %d exited\n", childPid);
            break;

    }
    // printf("The process with pid %d is returning from main\n", getpid());

}

void getInput(){
    printf(": "); //colon symbol as a prompt for each command line.
    fflush(stdout);

    //Variable Expansion
    while(1){
        ch = fgetc(stdin);
        if(ch == '$' && command[curSize-1] == '$'){
            command[curSize-1] = 0;
            char *pid;
            // int pidCounter = 0;
            asprintf(&pid, "%d", getpid());
            // printf("Here is the pid: %s\n", pid);
            // printf("Here is the length of the pid: %lu\n", strlen(pid));
            strcat(command, pid);
            curSize = curSize + (strlen(pid) - 1);
        }
        else{
            command[curSize++] = ch;
        }
        if(ch == '\n'){
            command[curSize] = '\0';
            break;
        }
    }
}

void clearInput(){
    memset(command, 0, curSize);
    curSize = 0;
}

void clearArgs(){
    memset(argList, 0, argSize);
    argSize = 0;
}


//Check for file Redirection
void fileRedirection(){
    int resultForeground;
    int resultBackgroundOut;
    int resultBackgroundIn;

    for(int i = 0; i < argSize; i++){
        if((strcmp(argList[i], ">") == 0) || (strcmp(argList[i], "<") == 0)){
            if(background == 1){
                targetFD = open("/dev/null", O_RDONLY);
                resultBackgroundOut = dup2(targetFD, STDOUT_FILENO);
                resultBackgroundIn = dup2(targetFD, STDIN_FILENO);
                
                if(resultBackgroundOut == -1 || resultBackgroundIn == -1){
                    perror("Error directing");
                    exit(1);
                }
            }
            else{
                if(strcmp(argList[i], ">") == 0){
                    // printf("OUTPUT HERE\n");
                    strcpy(outputFileName, argList[i+1]); //gets output filename only
                    // printf("OUTPUT FILE NAME: %s\n", outputFileName);
                    targetFD = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                    //Error in opening output file
                    if(targetFD == 1){
                        perror("target open()");
                        exit(1);
                    }
                    // printf("targetFD: %d", targetFD);
                    resultForeground = dup2(targetFD, STDOUT_FILENO);
                    if(resultForeground == -1){
                        perror("target dup2");
                        exit(2);
                    }
                    // printf("resultForeground: %d", resultForeground);
                    redirection = true; //Redirection occurs
                }
                else{
                    printf("ENTER HERE >");
                    // printf("INPUT HERE\n");
                    strcpy(inputFileName, argList[i+1]); //copy the string in the next index into inputFileName
                    targetFD = open(inputFileName, O_RDONLY); //open file
                    //error in opening file
                    if(targetFD == 1){
                        perror("source open()");
                        exit(1);
                    }
                    resultForeground = dup2(targetFD, STDIN_FILENO);
                    //erro
                    if(resultForeground == -1){
                        perror("source dup2");
                        exit(2);
                    }
                    redirection = true;
                }
            }
            fcntl(targetFD, F_SETFD, FD_CLOEXEC);
        }
    }
        //If < or > then zero out the rest of the array after the command.
        if(redirection){
            for(size_t i = 1; i < sizeof argSize; i++){
                argList[i] = NULL;
            }
        }
}