

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include "basic.h"


// Different function pointer types used by MR

#define MAX_NUM_ARGC 1024
#define MAX_LENGTH_ARGV 64
#define MAX_SIZE_RES 1024
#define PARTITION_NUM 5
#define NODE_NUM 100000
#define KEY_LENGTH 32
#define VALUE_LENGTH 32
#define QUEUE_SIZE 5

typedef struct bodynode{
    char * value;
    int visited;    
    struct bodynode * next;
} bodynode;

typedef struct headnode{
    char * key;
    char valid;
    bodynode * next;
    bodynode * tail;
    bodynode * cur;
} headnode;

typedef struct{
    // node content[NODE_NUM];
    // headnode ** table;
    headnode ** content;
    int len;
    char ** keys_copy;
} partition_t;

typedef struct {
    int front, rear, size;
    unsigned cap;
    int * argc_list;
    char ** argv_list;
    Mapper * mfuncs;
    Reducer * rfuncs;
    Getter * gfuncs;
    Partitioner * pfuncs;
    sem_t mutex, empty, full;
    // void (void**)(int argc, char ** argv);
} queue;

queue * init_queue(int cap);
int enqueue_reduce(queue *, Reducer, Partitioner );
int enqueue_map(queue *, Mapper, char*);

Reducer dequeue_reduce(queue *, Partitioner*);
Mapper dequeue_map(queue *, char **);
int queue_is_empty(queue * );
int queue_is_full(queue * );
// function_ptr front(queue * );
void f_reduce(char *key, Getter get_func, int partition_number);
void f_map(char*);

typedef struct {
    int num_thread;
    int avail_thread;
    pthread_t * threads;
    queue * q;
    int isMap;
    int jobNum, originJobNum; 
    void * args;
} thpool;

typedef struct{
    int thread_index;
    thpool * tp;
} thread_arg;

thpool * init_thpool(int, int, int, int);
void * routine_map(void * void_tp); 
void * routine_reduce(void * void_tp); 
int thpool_is_empty(thpool *);


partition_t * partitions;
char ** distinct_keys[NUM_REDUCERS];
int distinct_keys_size[NUM_REDUCERS];
int distinct_keys_cap[NUM_REDUCERS];
sem_t * sems;
sem_t * distinct_keys_sem;
Partitioner my_partitioner;