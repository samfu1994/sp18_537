

#ifndef THPOOL_H
#define THPOOL_H

#include "basic.h"
#include "mapreduce.h"

#define MAX_NUM_ARGC 1024
#define MAX_LENGTH_ARGV 64
#define MAX_SIZE_RES 1024
#define PARTITION_NUM 5
#define NODE_NUM 128
#define KEY_LENGTH 8
#define VALUE_LENGTH 8
#define REP_NUM 5

typedef struct{
    char * key;
    char * value;
} node;

typedef struct{
    node content[NODE_NUM];
    int len;
} partition;

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
    sem_t mutex, empty, full;
    // void (void**)(int argc, char ** argv);
} queue;

queue * init_queue(int cap);
int enqueue_reduce(queue *, Reducer, char*, Getter, int);
int enqueue_map(queue *, Mapper, char*);

Reducer dequeue_reduce(queue *, char**, Getter* , int*);
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
    int jobNum, originJobNum; 
} thpool;

thpool * init_thpool(int, int, int, int);
void * routine_map(void * void_tp); 
void * routine_reduce(void * void_tp); 
int thpool_add_job_reduce(thpool *, Reducer, int, char *, char * );
int thpool_add_job_map(thpool *, Mapper, char*);
int thpool_is_empty(thpool *);


partition * partitions;
sem_t * sems;
// unsigned long MR_DefaultHashPartition(char *key, int num_partitions);
// void MR_Emit(char *key, char *value);
#endif
