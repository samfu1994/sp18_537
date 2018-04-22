

#ifndef THPOOL_H
#define THPOOL_H

#include "basic.h"
#include "mapreduce.h"

#define MAX_NUM_ARGC 1024
#define MAX_LENGTH_ARGV 64
#define MAX_SIZE_RES 1024

typedef void (*mapper_ptr)(char*);
typedef void (*reducer_ptr)(char *key, Getter get_func, int partition_number);

typedef struct {
    int front, rear, size;
    unsigned cap;
    int * argc_list;
    char ** res_list;
    char ** argv_list;
    //char *** argv_list;
    mapper_ptr * mfuncs;
    reducer_ptr * rfuncs;
    sem_t mutex, empty, full;
    // void (void**)(int argc, char ** argv);
} queue;

queue * init_queue(int cap);
int enqueue_reduce(queue *, reducer_ptr, char*, Getter, int);
int enqueue_map(queue *, mapper_ptr, char*);

reducer_ptr dequeue_reduce(queue *, char**, Getter* , int*);
mapper_ptr dequeue_map(queue *, char *);
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
int thpool_add_job_reduce(thpool *, reducer_ptr, int, char **, char * );
int thpool_add_job_map(thpool *, mapper_ptr, char*);
int thpool_is_empty(thpool *);

#endif
