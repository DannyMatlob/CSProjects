#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include "bigbag.h"
#include <string.h>

//Returns the bigbag_entry formatted struct at selected offset
struct bigbag_entry_s *entry_addr(void *hdr, uint32_t offset) {
    if (offset == 0) return NULL;
    return (struct bigbag_entry_s *)((char*)hdr + offset);
}

//Returns the offset of this selected entry struct
uint32_t entry_offset(void *hdr, void *entry) {
    return (uint32_t)((uint64_t)entry - (uint64_t)hdr);
}

void dumpHdr(struct bigbag_hdr_s* hdr) {
    printf("Magic is %x\n",be32toh(hdr->magic));
    printf("firstFree is %i\n",hdr->first_free);
    printf("firstElement is %i\n",hdr->first_element);
    return;
}
void dumpEntry(struct bigbag_entry_s* entry) {
    printf("entry magic: %x\n", entry->entry_magic);
    printf("entry next offset: %i\n", entry->next);
    printf("entry len: %i\n", entry->entry_len);
    printf("entry string: %s\n", entry->str);
    printf("------------------------------\n");
}
struct bigbag_hdr_s *openFile(int persistent, char* fileName) {
    //If file doesn't exist, create new one, otherwise open file descriptor
    int fd = open(fileName, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        perror(fileName);
    }
    ftruncate(fd, BIGBAG_SIZE);
    if (ftruncate(fd, BIGBAG_SIZE)==-1) perror(fileName);

    //Map the file descriptor to a memory map, changing shared and private based on persistence requirement
    struct bigbag_hdr_s* file_base;
    if (persistent) {
        file_base = (struct bigbag_hdr_s*) mmap(0, BIGBAG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    } else {
        file_base = (struct bigbag_hdr_s*) mmap(0, BIGBAG_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    }

    if (file_base == MAP_FAILED) {
        perror("mmap");
        exit(3);
    }
    //Check the magic number to see if the file is already formatted properly, if not format it
    if (file_base->magic==NULL) {
        //printf("New file created\n");
        file_base->magic = be32toh(BIGBAG_MAGIC);
        file_base->first_element = 0;
        file_base->first_free = 12;
    }

    struct bigbag_entry_s *freeEntry = entry_addr(file_base, file_base->first_free);
    if (freeEntry!=NULL) {
        freeEntry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
        freeEntry->entry_len = BIGBAG_SIZE - file_base->first_free - 7;
    }
    //dumpHdr(file_base);
    return file_base;
}

struct bigbag_entry_s* bagAlloc(struct bigbag_hdr_s *hdr, char* string) {
    int stringLength = strlen(string);
    struct bigbag_entry_s *allocated = entry_addr(hdr,hdr->first_free);
    int entryLength = stringLength +7 + 1;
    if (entryLength + hdr->first_free > BIGBAG_SIZE) {
        return NULL;
    }
    allocated->entry_magic = BIGBAG_USED_ENTRY_MAGIC;
    allocated->next = 0;
    allocated->entry_len = stringLength+1;


    //Loop over memory and insert string
    int stringStart = hdr->first_free+7;
    char* file = (char*) hdr;
    //strcpy(file[stringStart],string);

    for (int i = stringStart; i<stringStart+stringLength; i++ ) {
        file[i] = string[i-stringStart];
    }
    hdr->first_free += entryLength;

    struct bigbag_entry_s *freeEntry = entry_addr(hdr, hdr->first_free);
    if (freeEntry!=NULL) {
        freeEntry->entry_magic = BIGBAG_FREE_ENTRY_MAGIC;
        freeEntry->entry_len = BIGBAG_SIZE - hdr->first_free - 7;
    }
    return allocated;
}

void addEntry(struct bigbag_hdr_s *hdr, char* string) {
    struct bigbag_entry_s *newEntry = bagAlloc(hdr, string);
    //Alloc failed, no space left
    if (newEntry==NULL) {
        printf("out of space\n");
        return;
    }
    printf("added %s\n" ,string);
    //edge case if no elements have been added before
    if (hdr->first_element==0) {
        hdr->first_element = entry_offset(hdr,newEntry);
        //printf("Inserted in the head of linkedlist, no prior elements\n");
        //dumpEntry(newEntry);
        return;
    }

    //Walk through linked list to insert in sorted order
    struct bigbag_entry_s *p1 = entry_addr(hdr, hdr->first_element);
    struct bigbag_entry_s *p2 = entry_addr(hdr, p1->next);

    //edge case 1, insertion into head of linked list
    if (strcmp(newEntry->str, p1->str)<0) {
        newEntry->next = entry_offset(hdr,p1);
        hdr->first_element = entry_offset(hdr,newEntry);
        //printf("Inserted in the head of linkedlist\n");
        //dumpEntry(newEntry);
        return;
    }

    //general case
    while (entry_offset(hdr,p2)<BIGBAG_SIZE) {

        //printf("p1 updated to: \n");
        //dumpEntry(p1);
        //printf("p2 updated to: \n");
        //dumpEntry(p2);
        if (strcmp(newEntry->str, p2->str)<=0) {
            p1->next = entry_offset(hdr,newEntry);
            newEntry->next = entry_offset(hdr,p2);
            //printf("Inserted in the middle of linkedlist between %s and %s\n", p1->str, p2->str);
            //dumpEntry(newEntry);
            return;
        }
        p1 = entry_addr(hdr, p1->next);
        p2 = entry_addr(hdr, p2->next);
    }
    p1->next = entry_offset(hdr,newEntry);
    //printf("Inserted in the end of linkedlist\n");
    //newEntry->next=hdr->first_free;
    //dumpEntry(newEntry);
}

void checkBag(struct bigbag_hdr_s *hdr, char* string) {
    //printf("Checking for %s\n" ,string);
    struct bigbag_entry_s *cur = entry_addr(hdr,hdr->first_element);
    //dumpEntry(cur);
    if (strcmp(cur->str, string)==0) {
        printf("found\n");
        return;
    }
    //maybe fix this if broken
    while(cur = entry_addr(hdr,cur->next)) {
        //dumpEntry(cur);
        if (strcmp(cur->str, string)==0) {
            printf("found\n");
            return;
        }
    }
    printf("not found\n");
}
void listAllStrings(struct bigbag_hdr_s *hdr) {
    if (hdr->first_element==0) {
        printf("empty bag\n");
        return;
    }
    struct bigbag_entry_s *cur = entry_addr(hdr,hdr->first_element);
    while(cur->next>0) {
        printf("%s\n", cur->str);
        cur = entry_addr(hdr,cur->next);
    }
    printf("%s\n", cur->str);
}

int main(int argc, char **argv) {
    //Error checking command usage
    if (argc<=1 || argc > 3 || (argc == 2 && strcmp(argv[1], "-t")==0)) {
        printf("USAGE: ./bigbag [-t] filename\n");
        exit(1);
    }
    //Open the file, or create a new one and map it to memory
    struct bigbag_hdr_s *hdr;
    if (strcmp(argv[1],"-t")==0) {
        hdr = openFile(0, argv[2]);
        //printf("Non-Persistent\n");
    } else {
        hdr = openFile(1, argv[1]);
        //printf("Persistent\n");
    }

    //Begin waiting for user input
    char *command = NULL;
    size_t stringLength = 0;
    int cmdLength;
    while (1) {
        //Get line, check for ctrl D
        int whyAreYouLikeThis = getline(&command, &stringLength, stdin);
        if (whyAreYouLikeThis == -1) return 0;

        if ((cmdLength = strlen(command))>2) {
            if (command[0]=='a' && command[1]==' ') {
                command[cmdLength] = '\0';
                command[cmdLength-1] = '\0';
                addEntry(hdr, command+2);
            } else if (command[0]=='c' && command[1]==' ') {
                command[cmdLength] = '\0';
                command[cmdLength-1] = '\0';
                checkBag(hdr,command+2);
            } else {
                printf("%c not used correctly\npossible commands:\na string_to_add\nd string_to_delete\nc string_to_check\nl\n", command[0]);
            }
        } else if (command[0]=='l') {
            listAllStrings(hdr);
        } else{
            printf("%c not used correctly\npossible commands:\na string_to_add\nd string_to_delete\nc string_to_check\nl\n", command[0]);
        }
        free(command);
        command=NULL;
    }
}