#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>
#include <time.h>

void printCmdList(char *cmdList[100][100]);

int main(int argc, char **argv) {
    if (argc <=1) { //Error checking if no args given
        printf("USAGE: ./app-sitter program0 space_separated_args0 . program1 space_separated_args1 ...\n");
        return 1;
    }

    //initializing command holder arrays
    char *cmd[100];
    char *cmdList[100][100];

    for (int i = 0; i<100; i++) {
        for (int j = 0; j<100; j++) {
            cmdList[i][j]=NULL;
        }
    }
    for (int i = 0; i<100; i++) {
        cmd[i] = NULL;
    }
    int curArg = 0;
    int childCounter = -1;
    time_t childStart[argc];
    pid_t children[argc];

    //Loop through all the args and seperate through "."
    for (int i = 1; i<=argc; i++) {
        //printf("i = %i, argc = %i, argv = %s\n", i, argc, argv[i]);
        if (cmd[0]!=NULL && (i==argc || (strcmp(argv[i],".")==0))) { // If argv is  a "." or its the last arg, perform fork exec
            //printf("cmd[0]!=null: %i, i==argc: %i, strcmp: %i\n", cmd[0]!=NULL, i==argc, strcmp(argv[i],".")==0);
            //printf("conditional: %i\n", cmd[0]!=NULL && (i==argc || (strcmp(argv[i],".")==0)));
            //printf(". detected, creating child %i\n\n", childCounter+1);
            cmd[curArg+1] = NULL;
            childCounter++;
            children[childCounter] = fork();
            if (children[childCounter]==0) {//Fork case for the child
                //Change file descriptor of process
                char out[20];
                char err[20];
                sprintf(out, "%i.out", childCounter);
                sprintf(err, "%i.err", childCounter);
                int fdOut = open(out, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                int fdErr = open(err, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                if (fdOut == -1) {perror(out);_exit(2);}//error check fdOut
                if (fdErr == -1) {perror(err);_exit(2);}//error check fdErr
                dup2(fdOut, 1);
                dup2(fdErr, 2);

                //Exec to run command in child
       //         printf("Attempting to exec child %i with command %s %s\n", childCounter, cmd[0], cmd[1]);
                execvp(cmd[0],cmd);
                perror(cmd[0]);
                //printf("exec failed, child\n");
                _exit(2); //If exec fails, we return error
            } else {//Parent handling child information
                int error=0;
                waitpid(children[childCounter],&error, WUNTRACED | WNOHANG);
                //printf("error is %i\n", error);
                if (error==0) {
                    //printf("Creating new child entry %i in parent\n", childCounter);
                    childStart[childCounter] = time(NULL); //Store time child started

                    //printf("Copying contents of cmd to cmdList\n\n");
                    for (int i = 0; i<argc; i++) { //Store cmd for later use
                        //printf("i = %i, sizeof cmd = %li\n", i, sizeof argc);
                        if (cmd[i]!=NULL) {
                            //printf("Copying %s\n", cmd[i]);
                            cmdList[childCounter][i] = strdup(cmd[i]);
                            //sprintf(cmdList[childCounter][i],"%s", cmd[i]);
                        } else {
                            break;
                        }
                    }
                    //printCmdList(cmdList);

                    //cmdList[childCounter] = cmd; //Store cmd for later use
                    //printf("Storing command %s %s into cmdList %s %s at counter %i\n" ,cmd[0],cmd[1],cmdList[childCounter][0],cmdList[childCounter][1], childCounter);

                   //printf("Resetting cmd for child counter: %i\n", childCounter);
                    for (curArg; curArg>=0; curArg--) { //Reset cmd
                        cmd[curArg] = NULL;
                    }
                    curArg++;//Off by one error
                } else {
                    childCounter--;
                    //printf("main childCounter is now %i\n",childCounter);
                    //printf("Command failed to run\n");
                }
            }
            //
        } else if (argv[i]!=NULL && strcmp(argv[i],".")!=0) { //else we add the arg to cmd
            //printf("cmd[0]!=null: %i, i==argc: %i, strcmp: %i\n", cmd[0]!=NULL, i==argc, strcmp(argv[i],".")==0);
            //printf("conditional: %i\n", cmd[0]!=NULL && (i==argc || (strcmp(argv[i],".")==0)));
            cmd[curArg] = argv[i];
            //printf("No . detected, storing: %s into cmd[%i] : %s\n\n", argv[i], curArg, cmd[curArg]);
            curArg++;
        }
    }

    //Waiting and Restarting portion
    //printCmdList(cmdList);
    //printf("Waiting for %i children\n", childCounter+1);
    int exitCode = 0;
    int child = 0;
    int childNumber = 0;
    time_t currentTime;
    while (childCounter>=0) {
        child = wait(&exitCode);//Watch for next child to exit

        //Loop through children to find which child this is
        for (int i = 0; i<sizeof children; i++) {
            if (children[i]==child) {
                childNumber = i;
                break;
            }
        }
        //Output exit code
        char out[20];
        char err[20];
        sprintf(out, "%i.out", childNumber);
        sprintf(err, "%i.err", childNumber);
        int fdOut = open(out, O_WRONLY | O_APPEND | S_IRUSR | S_IWUSR);
        int fdErr = open(err, O_WRONLY | O_APPEND | S_IRUSR | S_IWUSR);
        if (fdOut == -1) {perror(out);_exit(2);}//error check fdOut
        if (fdErr == -1) {perror(err);_exit(2);}//error check fdErr
        dup2(fdOut, 1);
        dup2(fdErr, 2);

        if (WIFEXITED(exitCode)) {
            fprintf(stderr,"exited rc = %i\n", WEXITSTATUS(exitCode));
        } else {//Output signal
            fprintf(stderr,"signal %i\n", WTERMSIG(exitCode));
        }

        currentTime = time(NULL);//update current time

        //Check if process has lived longer than 3 seconds, restart, else remove child
        if (currentTime - childStart[childNumber] >= 3) {
            children[childNumber] = fork();
            if (children[childNumber] == 0) {//Child process
                //Change file descriptor of process
                char out[20];
                char err[20];
                sprintf(out, "%i.out", childNumber);
                sprintf(err, "%i.err", childNumber);
                int fdOut = open(out, O_WRONLY | O_APPEND | S_IRUSR | S_IWUSR);
                int fdErr = open(err, O_WRONLY | O_APPEND | S_IRUSR | S_IWUSR);
                if (fdOut == -1) {perror(out);_exit(2);}//error check fdOut
                if (fdErr == -1) {perror(err);_exit(2);}//error check fdErr
                dup2(fdOut, 1);
                dup2(fdErr, 2);

                //Exec
                execvp(cmdList[childNumber][0],cmdList[childNumber]);
                perror(cmdList[childNumber][0]);
                _exit(2);
            } else {//Parent process
                childStart[childNumber] = time(NULL);
                //printf("Restarting process %i with command: %s at time %li\n",childNumber ,cmdList[childNumber][0], childStart[childNumber]);
            }
        } else {
            //printf("child %i running command %s %s is spawning too fast\n", childNumber, cmdList[childNumber][0], cmdList[childNumber][1]);
            fprintf(stderr,"spawning too fast\n");
            //Free memory in cmdlist
            for (int i = 0; i<argc; i++) {
                if (cmdList[childNumber][i]!=NULL) {
                    //printf("Freeing cmd: %s\n" ,cmdList[childNumber][i]);
                    free(cmdList[childNumber][i]);
                } else {
                    break;
                }
            }
            childCounter--;
        }
    }
}


void printCmdList(char *cmdList[100][100]) {
    printf("\nPrinting contents of cmdList\n");
    for (int i = 0; i<5; i++) {
        if (cmdList[i][0]==NULL) break;
        printf("Command %i is ", i);
        for (int j = 0; j<5; j++) {
            printf("%s ", cmdList[i][j]);
        }
        printf("\n");
    }
    printf("done printing\n\n");
}