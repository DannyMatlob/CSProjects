#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define BUCKET_COUNT 256

//Linked List and Functions
struct linked_list_node {
    char* data;
    struct linked_list_node* next;
};

int addToLinkedlist(struct linked_list_node** head, char* word) {
    for (struct linked_list_node* cur = *head; cur!=NULL; cur = cur->next) {//Walk down list
        if (strcmp(cur->data,word)==0) return 0;//If we find the same word, return
    }

    //Add item to the beginning of linked list
    struct linked_list_node *new_node = malloc(sizeof *new_node); //Create new node
    new_node->data = strdup(word); //Allocate memory for this node's data and add data
    new_node->next = *head;//This node will be the new head so it points to the old head
    *head = new_node;
    return 1;
}

//Hashset and Functions
struct hashset {
    int items[BUCKET_COUNT];
    pthread_mutex_t mutexList[BUCKET_COUNT];
    struct linked_list_node *buckets[BUCKET_COUNT];
};

struct hashset wordSet;

int hash(char* word) {
    int total = 0;
    for (int i = 0; word[i]!='\0'; i++) {
        total = (total + (unsigned char) word[i]) % BUCKET_COUNT;
    }
    return total;
}

void addToHashset(struct hashset *set, char* word) {
    int h = hash(word);
    //If we add to the hashset, increment the items count
    pthread_mutex_lock(&set->mutexList[h]);
    if (addToLinkedlist(&set->buckets[h], word)!=0) { //only increment item count if add is successful
        set->items[h]++;
        //printf("Word %s added successfully\n", word);
    }
    pthread_mutex_unlock(&set->mutexList[h]);
}

void* addFileToHashset(void* fileName) {
    fileName = (char*) fileName;
    FILE *fp = fopen(fileName, "r");
    if (fp==NULL) {
        perror(fileName);
        return 0; //file not found
    }
    char *buffer;
    while (fscanf(fp, "%ms", &buffer)==1) {
        addToHashset(&wordSet, buffer);
        free(buffer);
    }
}

int totalOfHashset() {
    int total = 0;
    for (int i = 0; i<BUCKET_COUNT; i++) {
        total+=wordSet.items[i];
    }
    return total;
}
//Main
int main(int argc, char** argv) {
    //Prepare threads
    pthread_t threads[argc-1];
    memset(threads, 0, sizeof(threads));
    //printf("total threads: %li\n", sizeof(threads)/sizeof(threads[0]));

    //Initialize Hashset
    memset(wordSet.buckets, 0, sizeof(wordSet.buckets));

    for (int i = 1; i<argc; i++) {
        //printf("argc is: %i, i = %i,  ", argc, i);
        //printf("my arg is: %s\n", argv[i]);
        FILE *fp = fopen(argv[i], "r");
        //printf("fp is: %i\n", fp);
        //printf("opening file %s with fp = %i\n", argv[i],fp);

        if (fp==NULL) {
            perror(argv[i]);
            //
        } else {
            pthread_create(&threads[i-1], NULL, addFileToHashset, argv[i]);
        }
        close(fp);
    }
    for (int i = 0; i < argc-1; i++) {
        //printf("argc-1 = %i, Waiting for Thread[%i]: %i\n",argc-1,i,threads[i]);
        pthread_join(threads[i],NULL);
        //printf("Thread: %i, joined\n", threads[i]);
    }
    printf("%i\n", totalOfHashset());
}
