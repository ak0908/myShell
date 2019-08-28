// assgn-1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>  
#include <errno.h>
#include <string.h>
#include <fcntl.h>
extern char **getLine();

void execute(char** args){
    pid_t pid;
    int s, fd, i;
    pid = fork();

    if(pid < 0){
        perror("fork");
        exit(1);
    }

    else if(pid == 0){
        for(i = 0; args[i] != NULL; i++){
            if(strcmp(args[i], ">") == 0){
                fd = creat(args[i+1], 0666);
                close(1);
                dup(fd);
                close(fd);
                args[i] = NULL;   
                break;             
            }
            else if(strcmp(args[i], ">>") == 0){
                fd = open(args[i+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
                close(1);
                dup(fd);
                close(fd);  
                args[i] = NULL; 
                break;                
            }
            else if(strcmp(args[i], "<") == 0){
                fd = open(args[i+1], O_CREAT | O_RDONLY, 0666);
                close(0);
                dup(fd);
                close(fd);
                args[i] = args[i+1];
                args[i+1] = NULL;
                break;
            }
        }

        if(execvp(args[0], args) == -1){
            perror("execvp");
            exit(1);
        }    
    }

    else {
        if(wait(&s) == -1){
            perror("wait");
        }
    }
}


void piping(char*** argsList, int numPipe){
    int p[2];
    int i;
    pid_t lpid, mpid, rpid;
    pipe(p);

    lpid = fork();
    if(lpid < 0){
        perror("fork");
        exit(1);
    }

    if(lpid == 0){
        dup2(p[1], 1);
        close(p[1]); close(p[0]);
        if(execvp(argsList[0][0], argsList[0]) == -1){
            perror("Error");
            exit(1);        
        }  
    }

    int temp_in = p[0]; // I will keep this p[0] for the next child.
    close(p[1]);        // no longer need p[1] this for output since next child would use the newlly created pipe

    for(i = 1; i < numPipe; i++){
        pipe(p);
        mpid = fork();

        if(mpid < 0){
            perror("fork");
            exit(1);
        }

        if(mpid == 0){

            dup2(temp_in, 0);
            close(temp_in);
            close(p[0]);

            dup2(p[1], 1);
            close(p[1]);


            if(execvp(argsList[i][0], argsList[i]) == -1){
                perror("execvp");
                exit(1);           
            }

        }

        close(p[1]);      // I will keep this p[0] for the next child.
        temp_in = p[0];   // no longer need p[1] this for output since next child would use the newlly created pipe
    }


    rpid = fork();
    if(rpid < 0){
        perror("fork");
        exit(1);
    }

    if(rpid == 0){
        dup2(temp_in, 0);
        close(temp_in); 
        if(execvp(argsList[numPipe][0], argsList[numPipe]) == -1){
            perror("execvp");
            exit(1);
        }
    }


    if(waitpid(rpid, (void *)0, 0) == -1){
        perror("Error");
        exit(1);
    }

}

void executeWithSemi(char*** argsList, int numSemi){
    int i;
    for(i = 0; i <= numSemi; i++){
        execute(argsList[i]);
    }
}



int main() {
    int i, j, k, numPipe, numSemi;
    char **args;
    char ***argsList;
    printf("$ ");

    while(1) {
        args = getLine();

        if(args[0] == NULL){
            printf("%s", "$ ");
            continue;
        }


        // exit
        if(strcmp(args[0], "exit") == 0){
            exit(0);
        } 

              
        numPipe = 0;
        numSemi = 0;
        for(i = 0; args[i] != NULL; i++) {
            // printf("%s\n", args[i]);
            if(strcmp(args[i], "|") == 0){
                numPipe ++;
            }

            if(strcmp(args[i], ";") == 0 || strcmp(args[i], "&&") == 0){
                numSemi ++;
            }
        } 

        // command with pipes
        if(numPipe > 0){
            argsList = (char***)calloc(numPipe + 1, sizeof(char**));
            for(i = 0; i <= numPipe; i++) argsList[i] = (char**)calloc(100, sizeof(char*));
            i = 0;
            j = 0;
            k = 0;
            while(args[i] != NULL){
                if(strcmp(args[i], "|") == 0){
                    i ++;
                    argsList[j][k] = NULL;
                    j ++;
                    k = 0;
                    continue;
                }
                argsList[j][k] = strdup(args[i]);
                i ++;
                k ++;
            }
            argsList[j][k] = NULL;

            piping(argsList, numPipe);

            // need to free the memory after strdup
            for(i = 0; i <= numPipe; i++){
                for(j =0; argsList[i][j] != NULL; j++) free(argsList[i][j]);
               free(argsList[i]); 
            } 
            free(argsList);
        } 

        // command with semi 
        else if(numSemi > 0){
            argsList = (char***)calloc(numSemi + 1, sizeof(char**));
            for(i = 0; i <= numSemi; i++) argsList[i] = (char**)calloc(100, sizeof(char*));
            i = 0;
            j = 0;
            k = 0;
            while(args[i] != NULL){
                if(strcmp(args[i], ";") == 0 || strcmp(args[i], "&&") == 0){
                    i ++;
                    argsList[j][k] = NULL;
                    j ++;
                    k = 0;
                    continue;
                }
                argsList[j][k] = strdup(args[i]);
                i ++;
                k ++;
            }
            argsList[j][k] = NULL;

            executeWithSemi(argsList, numSemi);

            // need to free the memory after strdup
            for(i = 0; i <= numSemi; i++){
                for(j =0; argsList[i][j] != NULL; j++) free(argsList[i][j]);
               free(argsList[i]); 
            } 
            free(argsList);

        }

        // cd
        else if(strcmp(args[0], "cd") == 0){
            if(args[1] == NULL) {
                if(chdir(getenv("HOME")) == -1){
                    perror("cd");
                }
            }
            else{
                if(chdir(args[1]) == -1){
                    perror("cd");
                }
            } 
        }

        // rest of commands
        else{
            execute(args);
        }

        // need to free the memory before calling getLine()
        for(i = 0; args[i] != NULL; i++) free(args[i]);

        printf("$ ");
    }
}       
