#ifndef __mapreduce_h__
#define __mapreduce_h__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include "basic.h"


// Different function pointer types used by MR
typedef char *(*Getter)(char *key, int partition_number);
typedef void (*Mapper)(char *file_name);
typedef void (*Reducer)(char *key, Getter get_func, int partition_number);
typedef unsigned long (*Partitioner)(char *key, int num_partitions);

// External functions: these are what you must define
void MR_Emit(char *key, char *value);

unsigned long MR_DefaultHashPartition(char *key, int num_partitions);

void MR_Run(int argc, char *argv[], 
	    Mapper map, int num_mappers, 
	    Reducer reduce, int num_reducers, 
	    Partitioner partition);

#define MAX_NUM_ARGC 1024
#define MAX_LENGTH_ARGV 64
#define MAX_SIZE_RES 1024
#define PARTITION_NUM 5
#define NODE_NUM 128
#define KEY_LENGTH 128
#define VALUE_LENGTH 128
#define REP_NUM 5
#define DISTINCT_KEY_NUM 1024

typedef struct{
    char * key;
    char * value;
    int visited;
} node;

typedef struct{
    node content[NODE_NUM];
    int len;
} partition_t;

typedef void (*Mapper)(char*);
typedef void (*Reducer)(char *key, Getter get_func, int partition_number);

typedef struct {
    int front, rear, size;
    unsigned cap;
    int * argc_list;
    char ** res_list;
    char ** argv_list;
    //char *** argv_list;
    Mapper * mfuncs;
    Reducer * rfuncs;
    Getter * gfuncs;
    sem_t mutex, empty, full;
    // void (void**)(int argc, char ** argv);
} queue;

queue * init_queue(int cap);
int enqueue_reduce(queue *, Reducer);
int enqueue_map(queue *, Mapper, char*);

Reducer dequeue_reduce(queue *);
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
} thpool;

thpool * init_thpool(int, int, int, int);
void * routine_map(void * void_tp); 
void * routine_reduce(void * void_tp); 
int thpool_is_empty(thpool *);


partition_t * partitions;
char distinct_keys[128][128];
int distinct_keys_size;
int * cache;
sem_t * sems;
sem_t distinct_keys_sem;
#endif // __mapreduce_h__