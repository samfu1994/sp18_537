#include "mapreduce.h"
#include "myheader.h"

#define A 70123 /* Using prime numbers to avoid collision as I can */
#define B 76777
#define FIRSTH 37 

int get_hash_code(char * s) {
   unsigned h = FIRSTH;
   while (*s) {
     h = (h * A) ^ (s[0] * B);
     s++;
   }
   return h % NODE_NUM;
}

static int compare2(const void * a, const void * b){
    char ** n1 = (char**) a;
    char ** n2 = (char**) b;
    return -strcmp((*n1) , (*n2));
}

char * get_from_distinct_keys(int thread_index) {
    char * ptr;

    sem_wait(&distinct_keys_sem[thread_index]);
    if(distinct_keys_size[thread_index] == 0) {
        sem_post(&distinct_keys_sem[thread_index]);
        return NULL;
    }
    ptr = distinct_keys[thread_index][distinct_keys_size[thread_index] - 1];
    distinct_keys_size[thread_index] -= 1;
    sem_post(&distinct_keys_sem[thread_index]);
    return ptr;
}

void add_first_bodynode(partition_t * p, int index, char * key, char * value) {
    strcpy(p -> content[index] -> key, key);
    p -> content[index] -> next = malloc(sizeof(bodynode));
    p -> content[index] -> next -> value = malloc(sizeof(char) * VALUE_LENGTH);
    strcpy(p -> content[index] -> next -> value, value);
    p -> content[index] -> tail = p -> content[index] -> next;
    p -> content[index] -> tail -> next = NULL;
    p -> content[index] -> valid = 1;
    p -> keys_copy[p -> len] = malloc(sizeof(char) * KEY_LENGTH);
    strcpy(p -> keys_copy[p -> len], key);
    p -> content[index] -> cur = p -> content[index] -> next;
    return;
}

int insert_to_hash(char * key, char * value) {

    unsigned long partition_index = my_partitioner(key, NUM_REDUCERS);
    partition_t * p = &partitions[partition_index];
    sem_wait(&(sems[partition_index]));

    int index = get_hash_code(key);

    if(p -> content[index] -> valid == 0) {
        //no one is here
        add_first_bodynode(p, index, key, value);
        p -> len += 1;
    }
    else{
        //in case of collision
        while(  p -> content[index] -> valid == 1
                && strcmp(p -> content[index] -> key, key) != 0) {
                index = (index + 1) % NODE_NUM;
        }
        if(p -> content[index] -> valid == 1) {
            //some key is here
            if(strcmp(p -> content[index] -> key, key) != 0) {
                //not gonna happen
                printf("error is here, insert_to_hash\n");
            }
            else {
                //this key is here
                bodynode * newtail = malloc(sizeof(bodynode));
                p -> content[index] -> tail -> next = newtail;
                p -> content[index] -> tail = newtail;
                newtail -> value = malloc(sizeof(char) * VALUE_LENGTH);
                strcpy(newtail -> value, value);        
                newtail -> next = NULL;
            }
        }
        else{
            //collsion, but this key was not here.
            add_first_bodynode(p, index, key, value);
            p -> len += 1;
        }
    }
    sem_post(&(sems[partition_index]));
    return 0;
}

char * get_from_hash(partition_t * p, char * key) {
    int index = get_hash_code(key);
    while(  p -> content[index] -> valid == 1
            && strcmp(p -> content[index] -> key, key) != 0) {
        index = (index + 1) % NODE_NUM;
    }
    if(p -> content[index] -> valid == 0) {
        printf("Error : get_from_hash at index %d, no such key : %s, len %lu\n", index, key, strlen(key));
        exit(1);
    }
    else{
        bodynode * bn = p -> content[index] -> cur;
        if(!bn) {
            return NULL;
        }
        else {
            bn -> visited = 1;
            p -> content[index] -> cur = p -> content[index] -> cur -> next;
            return bn -> value;
        }
    }
    printf("get_from_hash: should not reach here\n");
    return NULL;
}

char * myGet(char * key, int partition_number) {
    partition_t * p = &partitions[partition_number];
    char * res = get_from_hash(p, key);
    return res;
}

void free_queue(queue * q) {
    free(q -> rfuncs);
    free(q -> mfuncs);
    free(q -> gfuncs);
    free(q -> pfuncs);
    free(q -> argc_list);
    free(q -> argv_list);
    free(q);
    return;
}

queue * init_queue(int cap) {
    queue * q = malloc(sizeof(queue));
    q -> cap = cap;
    q -> rfuncs = malloc(sizeof(Reducer) *  cap);
    q -> mfuncs = malloc(sizeof(Mapper) *  cap);
    q -> gfuncs = malloc(sizeof(Getter) *  cap);
    q -> pfuncs = malloc(sizeof(Partitioner) *  cap);
    q -> argc_list = malloc(sizeof(int) * cap);
    q -> argv_list = malloc(sizeof(char*) * cap); 
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

int enqueue_reduce(queue * q, Reducer fp, Partitioner partitioner) {
    sem_wait(&(q -> empty));
    sem_wait(&(q -> mutex));
    q -> size += 1;
    q -> rfuncs[q -> rear] = fp;
    q -> gfuncs[q -> rear] = myGet;
    q -> pfuncs[q -> rear] = partitioner;
    q -> rear = (q -> rear + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> full));
    return 0;
}

int enqueue_map(queue * q, Mapper fp, char * file) {
    // printf("enter enquqeue_map\n");
    sem_wait(&(q -> empty));
    sem_wait(&(q -> mutex));
    q -> size += 1;
    q -> mfuncs[q -> rear] = fp;
    q -> argv_list[q -> rear] = file;
    q -> rear = (q -> rear + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> full));
    return 0;
}

Reducer dequeue_reduce(queue * q, Partitioner * partitioner){
    int cur;
    Reducer r;
    int tmp_empty = -2, tmp_full = -2;
    sem_getvalue(&(q -> empty), &tmp_empty);
    sem_getvalue(&(q -> full), &tmp_full);
    sem_wait(&(q -> full));
    sem_wait(&(q -> mutex));
    q -> size -= 1;
    cur = q -> front;
    *partitioner = q -> pfuncs[cur];
    r = q -> rfuncs[cur];    
    q -> front = (q -> front + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> empty));
    return r;
}

Mapper dequeue_map(queue * q, char ** file){
    int cur;
    sem_wait(&(q -> full));
    sem_wait(&(q -> mutex));
    q -> size -= 1;
    cur = q -> front;
    *file = q -> argv_list[cur];
    q -> front = (q -> front + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> empty));
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
        if(tp -> jobNum != 0) {
            sleep(1);
        }
        else {
            for(int i = 0; i < tp -> num_thread; i++) {
                if(pthread_cancel(tp -> threads[i]) != 0){
                    // printf("killing thread %d error\n", i);
                    //Shoud be just one thread here, the one quit by itself.
                }
            }
            break;
        }
    }
    return NULL;
}

void free_thpool(thpool * tp) {
    free_queue(tp -> q);
    free(tp -> threads);
    free(tp -> args);
    free(tp);
}

thpool * init_thpool(int thread_num, int queue_cap, int isMap, int jobNum) {
    thpool * tp = malloc(sizeof(thpool));
    tp -> num_thread = thread_num;
    tp -> avail_thread = thread_num;
    tp -> q = init_queue(queue_cap);
    tp -> jobNum = jobNum ;
    tp -> originJobNum = jobNum ;
    tp -> isMap = isMap;
    tp -> threads = malloc(sizeof(pthread_t) * (thread_num + 1));
    void * (*routine)(void*);
    if(isMap){
        routine = &routine_map;
    }
    else{
        routine = &routine_reduce;
    }
    thread_arg * thread_args = malloc(sizeof(thread_arg) * thread_num);
    tp -> args = (void*)thread_args;
    for(int i = 0; i < thread_num; i++) {
        thread_args[i].tp = tp;
        thread_args[i].thread_index = i;
        pthread_create(&tp -> threads[i], NULL, routine, (void*) &thread_args[i]);
    }
    if(isMap)
        pthread_create(&(tp -> threads[thread_num]), NULL, cleaner,(void*) tp);
    return tp;
}

void * routine_map(void * cur_arg) {
    thpool * tp = ((thread_arg *) cur_arg) -> tp;
    // int index = ((thread_arg *) cur_arg) -> thread_index;
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
    return NULL;
}


void * routine_reduce(void * cur_arg) {
    thread_arg * arg = (thread_arg *)cur_arg;
    thpool * tp = arg -> tp;
    void * res = NULL;
    int thread_index = arg-> thread_index;
    while(1) {
        char * k = get_from_distinct_keys(thread_index);
        if(k == NULL) {
            // printf("no more distinct keys, quitting!\n");
            break;
        }
        Partitioner curPartition;
        Reducer fp = dequeue_reduce(tp -> q, &curPartition);
        (*fp)(k, myGet, curPartition(k, NUM_REDUCERS));
    }
    return res;
}

int thpool_is_empty(thpool * tp) {
    return queue_is_empty(tp -> q);
}

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Emit(char *key, char *value){
    insert_to_hash(key, value);
    return;
}

void free_partition(partition_t * p) {
    headnode ** h = p -> content;
    bodynode * b, *n;
    for(int i = 0; i < p -> len; i++) {
        free(p -> keys_copy[i]);
    }

    for(int i = 0; i < NODE_NUM; i++) {
        b = h[i] -> next;
        while(b){
            n = b -> next;
            free(b -> value);
            free(b);
            b = n;
        }
        free(h[i] -> key);//each head
        free(h[i]);
    }
    free(p -> keys_copy);
    free(h);//whole partition

}

void free_MR_run() {
    for(int i = 0; i < NUM_REDUCERS; i++) {
        free_partition(&(partitions[i]));
    }
    free(partitions);
    free(sems);
}

 
void MR_Run(int argc, char *argv[], 
        Mapper map, int num_mappers, 
        Reducer reduce, int num_reducers, 
        Partitioner partitioner){

    my_partitioner = partitioner;
    partitions = malloc(sizeof(partition_t) * NUM_REDUCERS);
    sems = malloc(sizeof(sem_t) * NUM_REDUCERS);
    distinct_keys_sem = malloc(sizeof(sem_t) * NUM_REDUCERS);
    for(int i = 0; i < NUM_REDUCERS; i++){
        distinct_keys_size[i] = 0;
        if(sem_init(&(distinct_keys_sem[i]), 0, 1) != 0){
            printf("sem_init error\n");
        }
    }
    for(int i = 0; i < NUM_REDUCERS; i++) {
        partitions[i].content = (headnode**) malloc(sizeof(headnode*) * NODE_NUM);
        partitions[i].keys_copy = (char**) malloc(sizeof(char*) * NODE_NUM);
        partitions[i].len = 0;
        if(sem_init(&sems[i], 0, 1) != 0) {
            printf("sem_init error\n");
        }
        for(int j = 0; j < NODE_NUM; j++) {
            partitions[i].content[j] = (headnode*)malloc(sizeof(headnode));
            partitions[i].content[j] -> key = (char*)malloc(sizeof(char) * KEY_LENGTH);
            partitions[i].content[j] -> valid = 0;
            partitions[i].content[j] -> next = NULL;
            // partitions[i].content[j].visited = 0;
        }
        // printf("partion %d len is %d\n", i, partitions[i].len);
    }

    int queue_size = QUEUE_SIZE;
    Mapper mfp = map;
    Reducer rfp = reduce;
    
    //map phase
    thpool * tp = init_thpool(num_mappers, queue_size, 1, argc - 1);
    // printf("after init thpool argc is %d\n", argc);
    for(int j = 1; j < argc; j++) {
        enqueue_map(tp -> q, mfp, argv[j]); 
    }
    for(int i = 0; i <= num_mappers;  i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    //printf("MR_RUN : mapping phase finished.\n");
    //sorting phase
    for(int i = 0; i < NUM_REDUCERS; i++) {
        distinct_keys[i] = malloc(sizeof(char*) * partitions[i].len);
        for(int j = 0; j < partitions[i].len; j++) {
            distinct_keys[i][j] = malloc(sizeof(char) * KEY_LENGTH);
        }
        qsort(partitions[i].keys_copy, partitions[i].len, sizeof(char*), compare2);
        for(int j = 0; j < partitions[i].len; j++) {
            if(distinct_keys_size[i] == 0 || strcmp(partitions[i].keys_copy[j], distinct_keys[i][distinct_keys_size[i] - 1]) != 0) {
                strcpy(distinct_keys[i][distinct_keys_size[i]], partitions[i].keys_copy[j]);
                distinct_keys_size[i] += 1;
            }
        }
    }

    int origin_keys_size = 0;
    for(int i = 0; i < NUM_REDUCERS; i++){
        origin_keys_size += distinct_keys_size[i];
    }

    //printf("MR_RUN : sorting phase finished.\n");
    //reduce phase
    thpool * tp2 = init_thpool(num_reducers, queue_size, 0, num_reducers);
    for(int j = 0; j < origin_keys_size; j++){
        enqueue_reduce(tp2 -> q, rfp, partitioner);
    }

    //printf("MR_Run : reducing phase finished.\n");
    for(int i = 0; i < num_reducers;  i++) {
        pthread_join(tp2 -> threads[i], NULL);
    }   

    for(int i = 0; i < NUM_REDUCERS; i++) {
        for(int j = 0; j < partitions[i].len; j++) {
            free(distinct_keys[i][j]);
        }
        free(distinct_keys[i]);
    }
    free(distinct_keys_sem);
    free_thpool(tp);
    free_thpool(tp2);
    free_MR_run();
    
    return;    
}
