#include "mapreduce.h"
#define TEST_NUM 16

// static int compare(const void * a, const void * b){
//     node * n1 = (node*) a;
//     node * n2 = (node*) b;
//     int res = strcmp(n1 -> key, n2 -> key);
//     return -res;
// }
 
void MR_Run(int argc, char *argv[], 
        Mapper map, int num_mappers, 
        Reducer reduce, int num_reducers, 
        Partitioner partition){

    partitions = malloc(sizeof(partition) * PARTITION_NUM);
    sems = malloc(sizeof(sem_t) * PARTITION_NUM);
    for(int i = 0; i < PARTITION_NUM; i++) {
        // partitions[i].content = (node*) malloc(sizeof(node) * NODE_NUM);
        partitions[i].len = 0;
        if(sem_init(&sems[i], 0, 1) != 0) {
        printf("sem_init error\n");
        }
        for(int j = 0; j < NODE_NUM; j++) {
            partitions[i].content[j].key = (char*)malloc(sizeof(char) * KEY_LENGTH);
            partitions[i].content[j].value = (char*)malloc(sizeof(char) * VALUE_LENGTH);
        }
        printf("parition %d len is %d\n", i, partitions[i].len);
    }

    int queue_size = 10;
    int num_files = 5;
    // Reducer rfp = &f_reduce;
    Mapper mfp = &f_map;
     
    //map phase
    thpool * tp = init_thpool(num_mappers, queue_size, 1, num_files);
    char ** filename = malloc(sizeof(partition) * PARTITION_NUM);
    for(int j = 0; j < num_files; j++) {
        filename[j] = malloc(sizeof(char) * MAX_LENGTH_ARGV);
        sprintf(filename[j], "%d", j);
        thpool_add_job_map(tp, mfp, filename[j]);
    }
    for(int i = 0; i < num_mappers;  i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    printf("MR_RUN : mapping phase finished\n");
    //soring phase

    for(int i = 0; i < PARTITION_NUM; i++) {
        printf("partition %d, len is %d\n", i, partitions[i].len);
        for(int j = 0; j < partitions[i].len; j++) {
            printf("partitions %d, element %d: %s, %s\n", i, j, partitions[i].content[j].key, partitions[i].content[j].value);
        }
    }

    // for(int i = 0; i < PARTITION_NUM; i++) {
    //     qsort(partitions[i].content, partitions[i].len, sizeof(node), compare);
    // }
    // printf("MR_RUN : soring phase finished\n");
    // for(int i = 0; i < PARTITION_NUM; i++) {
    //     printf("partition %d, len is %d\n", i, partitions[i].len);
    //     for(int j = 0; j < partitions[i].len; j++) {
    //         printf("partitions %d, element %d: %s, %s\n", i, j, partitions[i].content[j].key, partitions[i].content[j].value);
    //     }
    // }
    
    // // //reduce phase
    // thpool * tp2 = init_thpool(num_reducers, queue_size, 0, num_reducers);
    // // for(int j = 0; j < num_reducers; j++){
    // //     printf("add reducing job %d\n", j);
    // //     argv = malloc(sizeof(char*) * argc);
    // //     for(int i = 0; i < argc; i++) {
    // //         argv[i] = malloc(sizeof(char) * MAX_LENGTH_ARGV);
    // //         // sprintf(argv[i], "come here and reduce %d", i + j);
    // //         strcpy(argv[i], partitions[0].content[0].key);
    // //     }
    // //     thpool_add_job_reduce(tp2, rfp, argc, argv, NULL);
    // // }

    // argv = malloc(sizeof(char*) * num_reducers);
    // for(int j = 0; j < num_reducers; j++){
    //     printf("add reducing job %d\n", j);
    //     argv[j] = malloc(sizeof(char) * MAX_LENGTH_ARGV);
    //     sprintf(argv[j], "%d", j);
    //     // strcpy(argv[i], partitions[0].content[0].key);
    //     thpool_add_job_reduce(tp2, rfp, argc, argv[j], NULL);
    // }
    
    // sleep(3);
    // printf("main:: after sleep\n");
    // for(int i = 0; i < num_reducers;  i++) {
    //     pthread_join(tp2 -> threads[i], NULL);
    // }   
    
    return;    
}

int main(int argc, char * argv[]){
    MR_Run(argc, argv, NULL, 1, NULL, 10, MR_DefaultHashPartition);
    printf("finish all\n");
    return 0;
}
