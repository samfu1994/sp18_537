#include "mapreduce.h"
#define TEST_NUM 16


// #ifndef NUM_MAPPERS
//     #define NUM_MAPPERS 1
// #endif
// #ifndef NUM_REDUCERS
//     #define NUM_REDUCERS 1
// #endif

// #ifndef FILE_OUTPUT_SUFFIX
//     #define FILE_OUTPUT_SUFFIX ""
// #endif

// int output_fd[NUM_REDUCERS];

// void Map(char *file_name) {
//     FILE *fp = fopen(file_name, "r");
//     assert(fp != NULL);

//     char *line = NULL;
//     size_t size = 0;
//     while (getline(&line, &size, fp) != -1) {
//         MR_Emit(line, "1");
//     }
//     free(line);
//     fclose(fp);
// }

// void Reduce(char *key, Getter get_next, int partition_number) {
//     char *value;
//     while ((value = get_next(key, partition_number)) != NULL)
//         dprintf(output_fd[partition_number],"%s", key);
// }



static int compare(const void * a, const void * b){
    node * n1 = (node*) a;
    node * n2 = (node*) b;
    int res = strcmp(n1 -> key, n2 -> key);
    return res;
}

void myMap(char *file_name) {
    FILE *fp = fopen(file_name, "r");
    // assert(fp != NULL);

    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL) {
            MR_Emit(token, "1");
        }
    }
    free(line);
    fclose(fp);
}

void myReduce(char *key, Getter get_next, int partition_number) {
    int count = 0;
    char *value;
    while ((value = get_next(key, partition_number)) != NULL){
        count++;
    }
    printf("%s %d\n", key, count);
}

char * get_from_distinct_keys() {
    char * ptr;
    printf("DISTINCT KEYS: try to get from distinct_keys, size %d\n", distinct_keys_size);
    sem_wait(&distinct_keys_sem);
    if(distinct_keys_size == 0) {
        sem_post(&distinct_keys_sem);
        return NULL;
    }
    ptr = distinct_keys[distinct_keys_size - 1];
    distinct_keys_size -= 1;
    sem_post(&distinct_keys_sem);
    printf("DISTICT KEYS: got %s\n", ptr);
    return ptr;
}

char * myGet(char * key, int partition_number) {
    // printf("myGet: try to fetch %s\n", key);
    unsigned long partition_index = MR_DefaultHashPartition(key, PARTITION_NUM);
    // printf("partition_index = %lu\n",partition_index);
    partition_t * p = &partitions[partition_index];
    int start = 0, end = p -> len - 1;
    int mid = 0;
    while(start < end) {
        mid = (start + end) / 2;
        // printf("start %d, mid %d, end %d\n", start, mid, end);
        int cur = strcmp(p -> content[mid].key, key);
        if(cur < 0) {
            start = mid + 1;
        }
        else if(cur > 0) {
            end = mid - 1;
        }
        else{
            if(p -> content[mid].visited) {
                start = mid + 1;
            }
            else {
                end = mid;
            }
        }
    }
    // printf("key is %s, content[%d].key is %s\n", key, start, p -> content[start].key); 
    if(strcmp(key, p -> content[start].key) != 0 || p -> content[start].visited != 0) {
        return NULL;
    }
    char * res;
    sem_wait(&(sems[partition_index]));
    p -> content[start].visited = 1;
    res = p -> content[start].value;
    // printf("myGet : res is %s\n", res);
    sem_post(&(sems[partition_index]));
    return res;
}

queue * init_queue(int cap) {
    queue * q = malloc(sizeof(queue));
    q -> cap = cap;
    // q -> funcs = malloc(sizeof(function_ptr) *  cap);
    q -> rfuncs = malloc(sizeof(Reducer) *  cap);
    q -> mfuncs = malloc(sizeof(Mapper) *  cap);
    q -> gfuncs = malloc(sizeof(Getter) *  cap);
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

int enqueue_reduce(queue * q, Reducer fp) {
    // printf("enter enqueue_reduce\n");
    sem_wait(&(q -> empty));
    sem_wait(&(q -> mutex));
    q -> size += 1;
    q -> rfuncs[q -> rear] = fp;
    q -> gfuncs[q -> rear] = myGet;
    // q -> argv_list[q -> rear] = key;
    // q -> res_list[q -> rear] = res;
    q -> rear = (q -> rear + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> full));
    // printf("quit enqueue_reduce\n");
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

Reducer dequeue_reduce(queue * q){
    int cur;
    Reducer r;
    int tmp_empty = -2, tmp_full = -2;
    sem_getvalue(&(q -> empty), &tmp_empty);
    sem_getvalue(&(q -> full), &tmp_full);
    printf("DE: : empty is %d, full is %d\n", tmp_empty, tmp_full);
    sem_wait(&(q -> full));
    sem_wait(&(q -> mutex));
    printf("executing dequeue_reduce\n");
    q -> size -= 1;
    cur = q -> front;
    // * par = PARTITION_NUM
    // * key = q -> argv_list[cur];
    // * g =  q -> gfuncs[cur];
    r = q -> rfuncs[cur];    
    q -> front = (q -> front + 1) % (q -> cap);
    sem_post(&(q -> mutex));
    sem_post(&(q -> empty));
    printf("quit dequeue_reduce\n");
    return r;
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
        printf("origin is %d, now is %d, isMap is %d\n", tp -> originJobNum, tp -> jobNum, tp -> isMap);
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
    tp -> isMap = isMap;
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
    if(isMap)
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
        printf("start running map function\n");
        (*fp)(filename);
        printf("returning from map function\n");
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
    // char * res = malloc(sizeof(char) * MAX_SIZE_RES);
    char ** argv = malloc(sizeof(char*) * MAX_NUM_ARGC);
    for(int i = 0; i < MAX_NUM_ARGC; i++) {
        argv[i] = malloc(sizeof(char*) * MAX_LENGTH_ARGV);
    }
    while(1) {
        char * k = get_from_distinct_keys();
        if(k == NULL) {
            printf("no more distinct keys, quitting!\n");
            break;
        }
        Reducer fp = dequeue_reduce(tp -> q);

        printf("routine_reduce : key is now %s\n", k);
        (*fp)(k, myGet, PARTITION_NUM);
    }
    printf("quit reduce routine\n");
    return NULL;
}

int thpool_is_empty(thpool * tp) {
    return queue_is_empty(tp -> q);
}

void f_reduce(char * key, Getter get_func, int partition_number) {
    unsigned long partition_index = MR_DefaultHashPartition(key, PARTITION_NUM);
    partition_t * p = &partitions[partition_index];
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
    // printf("key is %d, partition index is %lu\n", atoi(key), partition_index);
    partition_t * p = &partitions[partition_index];

    // printf("l now is %d\n", p -> len);
    // printf("key length is %lu, val length is %lu\n", strlen(key), strlen(value));
    sem_wait(&(sems[partition_index]));
    int l = p -> len;
    for(int i = 0; i < PARTITION_NUM; i++) {
        // printf("partition : %d, length is %lu\n", i, partitions[i].len);
    }
    // p -> content[l].key = key;
    // p -> content[l].value = value;
    strcpy(p -> content[l].key, key);
    strcpy(p -> content[l].value, value);
    p -> len += 1;
    // printf("ptr is %p\n", p -> content[l].key);
    // printf("inserting value : %s to parition %lu, len %lu, p -> len is %d\n", p -> content[l].key, partition_index, strlen(p -> content[l].key), p -> len);
    sem_post(&(sems[partition_index]));
    return;
}

 
void MR_Run(int argc, char *argv[], 
        Mapper map, int num_mappers, 
        Reducer reduce, int num_reducers, 
        Partitioner partition){

    partitions = malloc(sizeof(partition) * PARTITION_NUM);
    sems = malloc(sizeof(sem_t) * PARTITION_NUM);
    // distinct_keys = malloc(sizeof(char*) * DISTINCT_KEY_NUM);
    distinct_keys_size = 0;
    if(sem_init(&distinct_keys_sem, 0, 1) != 0){
        printf("sem_init error\n");
    }
    for(int i = 0; i < DISTINCT_KEY_NUM; i++) {
        // distinct_keys[i] = malloc(sizeof(char) * KEY_LENGTH);
        // memset(distinct_keys[i], 0, sizeof(char) * KEY_LENGTH);
    }

    for(int i = 0; i < PARTITION_NUM; i++) {
        // partitions[i].content = (node*) malloc(sizeof(node) * NODE_NUM);
        partitions[i].len = 0;
        if(sem_init(&sems[i], 0, 1) != 0) {
            printf("sem_init error\n");
        }
        for(int j = 0; j < NODE_NUM; j++) {
            partitions[i].content[j].key = (char*)malloc(sizeof(char) * KEY_LENGTH);
            partitions[i].content[j].value = (char*)malloc(sizeof(char) * VALUE_LENGTH);
            partitions[i].content[j].visited = 0;
        }
        // printf("parition %d len is %d\n", i, partitions[i].len);
    }

    int queue_size = 100;
    // Reducer rfp = &f_reduce;
    // Mapper mfp = &f_map;
    Mapper mfp = map;
    Reducer rfp = reduce;
     
    //map phase
    thpool * tp = init_thpool(num_mappers, queue_size, 1, argc);
    // char ** filename = malloc(sizeof(char*) * PARTITION_NUM);
    for(int j = 1; j < argc; j++) {
        enqueue_map(tp -> q, mfp, argv[j]); 
    }
    for(int i = 0; i <= num_mappers;  i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    printf("MR_RUN : mapping phase finished\n");
    //sorting phase

    for(int i = 0; i < PARTITION_NUM; i++) {
        // printf("before qsort\n");
        qsort(partitions[i].content, partitions[i].len, sizeof(node), compare);
        // printf("after qsort\n");
        for(int j = 0; j < partitions[i].len; j++) {
            // printf("j is %d, distinct_keys_size is %d\n", j, distinct_keys_size);
            // printf("distinct_keys is %s | content key is %s\n", distinct_keys[distinct_keys_size - 1], partitions[i].content[j].key);
            // printf("l1 is %lu, l2 is %lu\n", strlen(distinct_keys[distinct_keys_size - 1]), strlen(partitions[i].content[j].key));
            if(distinct_keys_size == 0 || strcmp(partitions[i].content[j].key, distinct_keys[distinct_keys_size - 1]) != 0) {
                // printf("before strcpy i : %d, j : %d, distinct size : %d,  %s, %s\n",i, j, distinct_keys_size, distinct_keys[distinct_keys_size], partitions[i].content[j].key);
                // sprintf(distinct_keys[distinct_keys_size], "1");
                strcpy(distinct_keys[distinct_keys_size], partitions[i].content[j].key);
                // printf("len is %lu\n", strlen(distinct_keys[distinct_keys_size]));
                distinct_keys_size += 1;
                // printf("now size is %d\n", distinct_keys_size);
            }
        }
    }
    for(int i = 0; i < distinct_keys_size; i++) {
        printf("%s\n", distinct_keys[i]);
    }
    printf("MR_RUN : sorting phase finished\n");
    for(int i = 0; i < PARTITION_NUM; i++) {
        printf("partition %d, len is %d\n", i, partitions[i].len);
        for(int j = 0; j < partitions[i].len; j++) {
            printf("partitions %d, element %d: %s, %s\n", i, j, partitions[i].content[j].key, partitions[i].content[j].value);
        }
    }
    
    //reduce phase
    thpool * tp2 = init_thpool(num_reducers, queue_size, 0, num_reducers);
    printf("-------SIZE is %d\n", distinct_keys_size);
    argv = malloc(sizeof(char*) * distinct_keys_size);
    for(int j = 0; j < distinct_keys_size; j++){
        printf("-----------MR_Run :: add reducing job %d\n", j);
        enqueue_reduce(tp2 -> q, rfp);
    }
    
    // sleep(3);
    // printf("-----------MR_Run:: after sleep\n");
    for(int i = 0; i < num_reducers;  i++) {
        pthread_join(tp2 -> threads[i], NULL);
    }   
    
    return;    
}

// int main(int argc, char * argv[]){
//     MR_Run(argc, argv, myMap, 5, myReduce, 5, MR_DefaultHashPartition);
//     printf("finish all\n");
//     return 0;
// }
