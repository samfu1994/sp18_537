#include "thpool.h"
#include <errno.h>

queue * init_queue(int cap) {
    printf("init queue!!!!!------------\n");
    queue * q = malloc(sizeof(queue));
    q -> cap = cap;
    // q -> funcs = malloc(sizeof(function_ptr) *  cap);
    q -> rfuncs = malloc(sizeof(Reducer) *  cap);
    q -> mfuncs = malloc(sizeof(Mapper) *  cap);
    q -> argc_list = malloc(sizeof(int) * cap);
    q -> argv_list = malloc(sizeof(char*) * cap); 
    q -> res_list = malloc(sizeof(char*) * cap); 
    q -> front = 0;
    q -> rear = 0;
    q -> size = 0;

    if(sem_init(&(q -> mutex), 0, 1) != 0){
        printf("sem_init error %d\n", errno);
    }
    if(sem_init(&(q -> full), 0, 0) != 0){
        printf("sem_init error %d\n", errno);
    }
    if(sem_init(&(q -> empty), 0, cap) != 0){
        printf("sem_init error %d\n", errno);
    }
    return q;
}

int enqueue_reduce(queue * q, Reducer fp, char * key, Getter g, int par) {
    sem_wait(&(q -> empty));
    sem_wait(&(q -> mutex));
    q -> size += 1;
    q -> rfuncs[q -> rear] = fp;
    q -> argc_list[q -> rear] = par;
    q -> argv_list[q -> rear] = key;
    // q -> res_list[q -> rear] = res;
    q -> rear = (q -> rear + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> full));
    return 0;
}

int enqueue_map(queue * q, Mapper fp, char * file) {
    sem_wait(&(q -> empty));
    sem_wait(&(q -> mutex));
    // int tmp_empty = -2, tmp_full = -2;
    // sem_getvalue(&(q -> empty), &tmp_empty);
    // sem_getvalue(&(q -> full), &tmp_full);
    // printf("EEEEN : empty is %d, full is %d\n", tmp_empty, tmp_full);
    printf("cur is %d, enqueue_map : %s\n", q -> rear, file);
    q -> size += 1;
    q -> mfuncs[q -> rear] = fp;
    q -> argv_list[q -> rear] = file;
    for(int i = 0; i < REP_NUM; i++)
        printf("after enqueue_map, argv_list[%d] is %s\n", i, q -> argv_list[i]);
    q -> rear = (q -> rear + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> full));
    return 0;
}

Reducer dequeue_reduce(queue * q, char** key, Getter* g, int* par){
    int cur;
    sem_wait(&(q -> full));
    sem_wait(&(q -> mutex));
    printf("executing dequeue_reduce\n");
    q -> size -= 1;
    cur = q -> front;
    * par = q -> argc_list[cur];
    * key = q -> argv_list[cur];
    // * res = q -> res_list[cur];
    
    q -> front = (q -> front + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> empty));
    return q -> rfuncs[cur];
}

Mapper dequeue_map(queue * q, char ** file){
    int cur;
    // printf("try to dequeue\n");
    sem_wait(&(q -> full));
    sem_wait(&(q -> mutex));
    // int tmp_empty = -2, tmp_full = -2;
    // sem_getvalue(&(q -> empty), &tmp_empty);
    // sem_getvalue(&(q -> full), &tmp_full);
    // printf("DE: : empty is %d, full is %d\n", tmp_empty, tmp_full);
    q -> size -= 1;
    cur = q -> front;
    //* file = q -> argv_list[cur];
    // strcpy(file, q -> argv_list[cur]);
    *file = q -> argv_list[cur];
    printf("cur is %d, dequeue_map %s, argvlist : %s\n", cur, *file, q -> argv_list[cur]);
    q -> front = (q -> front + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> empty));
    // printf("finish DE\n");
    return q -> mfuncs[cur];
}

int queue_is_empty(queue * q) {
    int res;
    sem_wait(&(q -> mutex));
    res = q -> size == 0;
    sem_post(&(q -> mutex));
    return res;
}

int queue_is_full(queue * q) {
    int res;
    sem_wait(&(q -> mutex));
    res = q -> size == q -> cap;
    sem_post(&(q -> mutex));
    return res;
}

void * cleaner(void * void_tp) {
    thpool * tp = (thpool*) void_tp;
    while(1){
        printf("origin is %d, now is %d\n", tp -> originJobNum, tp -> jobNum);
        if(tp -> originJobNum == tp -> jobNum) {
            sleep(1);
        }
        else {
            printf("someone has finished\n");
            for(int i = 0; i < tp -> num_thread; i++) {
                if(pthread_cancel(tp -> threads[i]) != 0){
                    printf("killing thread %d error\n", i);
                }
            }
            break;
        }
    }
    return NULL;
}

thpool * init_thpool(int thread_num, int queue_cap, int isMap, int jobNum) {
    thpool * tp = malloc(sizeof(thpool));
    tp -> num_thread = thread_num;
    tp -> avail_thread = thread_num;
    tp -> q = init_queue(queue_cap);
    tp -> jobNum = jobNum;
    tp -> originJobNum = jobNum;
    tp -> threads = malloc(sizeof(pthread_t) * (thread_num + 1));
    void * (*routine)(void*);
    if(isMap){
        routine = &routine_map;
    }
    else{
        routine = &routine_reduce;
    }

    for(int i = 0; i < thread_num; i++) {
        pthread_create(&tp -> threads[i], NULL, routine, (void*) tp);
    }
    pthread_create(&(tp -> threads[thread_num]), NULL, cleaner,(void*) tp);
    return tp;
}

void * routine_map(void * void_tp) {
    thpool * tp = (thpool *) void_tp;
    // int argc;
    // char * res = malloc(sizeof(char) * MAX_SIZE_RES);
    // char ** argv = malloc(sizeof(char*) * MAX_NUM_ARGC);
    // for(int i = 0; i < MAX_NUM_ARGC; i++) {
    //     argv[i] = malloc(sizeof(char*) * MAX_LENGTH_ARGV);
    // }
    char * filename;
    while(1) {
        Mapper fp = dequeue_map(tp -> q, &filename);
        (*fp)(filename);
        sem_wait(&(tp -> q -> mutex));
        tp -> jobNum -= 1;
        if(tp -> jobNum == 0) {
            sem_post(&(tp -> q -> mutex));
            break;
        }
        sem_post(&(tp -> q -> mutex));
    }
    printf("quit map routine\n");
    return NULL;
}


void * routine_reduce(void * void_tp) {
    thpool * tp = (thpool *) void_tp;
    int argc;
    // char * res = malloc(sizeof(char) * MAX_SIZE_RES);
    char ** argv = malloc(sizeof(char*) * MAX_NUM_ARGC);
    for(int i = 0; i < MAX_NUM_ARGC; i++) {
        argv[i] = malloc(sizeof(char*) * MAX_LENGTH_ARGV);
    }
    while(1) {
        Reducer fp = dequeue_reduce(tp -> q, &argv[0], NULL, &argc);
        (*fp)(argv[0], NULL, argc);
        sem_wait(&(tp -> q -> mutex));
        tp -> jobNum -= 1;
        if(tp -> jobNum == 0) {
            sem_post(&(tp -> q -> mutex));
            break;
        }
        sem_post(&(tp -> q -> mutex));
    }
    printf("quit reduce routine\n");
    return NULL;
}

int thpool_add_job_reduce(thpool * tp, Reducer fp, int argc, char * argv, char * res) {
    // return enqueue(tp -> q, fp, argc, argv, res);
    return enqueue_reduce(tp -> q, fp, argv, NULL, argc); 
}

int thpool_add_job_map(thpool * tp, Mapper fp, char * filename) {
    // return enqueue(tp -> q, fp, argc, argv, res);
    printf("thread_add_job_map: %s\n", filename);
    return enqueue_map(tp -> q, fp, filename); 
}

int thpool_is_empty(thpool * tp) {
    return queue_is_empty(tp -> q);
}

void f_reduce(char * key, Getter get_func, int partition_number) {
    unsigned long partition_index = MR_DefaultHashPartition(key, PARTITION_NUM);
    partition * p = &partitions[partition_index];
    int start = 0, end = p -> len;
    int mid = 0;
    while(start < end) {
        mid = (start + end) / 2;
        int cur = strcmp(p -> content[mid].key, key);
        if(cur < 0) {
            start = mid + 1;
        }
        else if(cur > 0) {
            end = mid - 1;
        }
        else{
            break;
        }
    }
    start = mid;
    end = mid;
    printf("key is %s\n", key);
    printf("start is %d, end is %d\n", start, end);
    while(start >= 0 && strcmp(p -> content[start].key, key) == 0) {
        start -= 1;
    }
    start += 1;
    while(end <= p -> len && strcmp(p -> content[end].key, key) == 0) {
        end += 1;
    }
    end -= 1;
    int sum = 0;
    printf("start is %d, end is %d\n", start, end);
    for(int i = start; i <= end; i++) {
        printf("reduce :: %s,  %s\n", p -> content[i].key, p -> content[i].value);
        sum += atoi(p -> content[i].value);
    }
    printf("sum of %s is %d\n", key, sum);
}

void f_map(char* file) {
    char * value;
    char * key;
    for(int i = 0; i < REP_NUM; i++) {
        key = (char*)malloc(sizeof(char) * KEY_LENGTH);
        value = (char*)malloc(sizeof(char) * VALUE_LENGTH);
        sprintf(value, "2");
        sprintf(key, "0");
        // MR_Emit(file, value);
        MR_Emit(key, value);
    }
    printf("map : %s\n", file);
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Emit(char *key, char *value){
    unsigned long partition_index = MR_DefaultHashPartition(key, PARTITION_NUM);
    printf("key is %d, partition index is %lu\n", atoi(key), partition_index);
    partition * p = &partitions[partition_index];

    // printf("l now is %d\n", p -> len);
    // printf("key length is %lu, val length is %lu\n", strlen(key), strlen(value));
    sem_wait(&(sems[partition_index]));
    int l = p -> len;
    for(int i = 0; i < PARTITION_NUM; i++) {
        // printf("partition : %d, length is %lu\n", i, partitions[i].len);
    }
    p -> content[l].key = key;
    p -> content[l].value = value;
    // strcpy(p -> content[l].key, key);
    // strcpy(p -> content[l].value, value);
    p -> len += 1;
    printf("ptr is %p\n", p -> content[l].key);
    printf("inserting value : %s, len %lu, p -> len is %d\n", p -> content[l].key, strlen(p -> content[l].key), p -> len);
    sem_post(&(sems[partition_index]));
    return;
}
