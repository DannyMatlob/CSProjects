#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    //Handling Usage Errors
    if (argc<=1) {
        printf("USAGE: ./pccseq filename ...\n");
        return 1;
    }

    //All children will use this shared memory space to communicate with eachother
    int *sharedMax = mmap(NULL, 1, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    pid_t pid;
    for (int i = 1; i<argc; i++) {//Open children for each file
        //Open the file for reading and map it to a memory location
        int fd = open(argv[i],O_RDONLY);
        if (fd==-1) { //Incorrect File Selection
            perror(argv[i]);
            exit(2);
        }
        struct stat stat;
        if (fstat(fd, &stat) == -1) {
            perror(argv[i]);
            exit(2);
        }
        pid = fork();
        //if (pid!=0) {printf("Forked, This program has a pid of %i\n", pid );}
        if (pid==0) {
            //Create the memory map containing the file
            char *ptr = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

            //Variables for finding longest sequence
            int longest = 0;
            int current = 0;

            //Figure out longest sequence of 0s in a byte
            for (int i = 0; i<stat.st_size; i++) {
                char c = ptr[i];
                for (int j = 0; j<8; j++) {
                    if ((c&128)==0) {
                        current++;
                    } else {
                        current = 0;
                    }
                    if (current>longest) longest=current;
                    c=c<<1;
                }
            }
            close(fd);
            //printf("pid: %i, found a value of %i, setting sharedMax and exiting\n",pid, longest);
            if (*sharedMax<longest) {
                *sharedMax = longest;
                //printf("sharedMax is now %i\n", *sharedMax);
            }
            return 0;
        }
    }
    //Parent will wait for children to exit then print the longest sequence of 0s.
    while (wait(NULL)>0) {}//Wait for all children to exit
    printf("%i\n",*sharedMax );
    return 0;
}
