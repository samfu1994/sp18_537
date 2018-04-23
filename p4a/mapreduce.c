#include "thpool.h"

#define TEST_NUM 16

void MR_Run(int argc, char *argv[], 
        Mapper map, int num_mappers, 
        Reducer reduce, int num_reducers, 
        Partitioner partition){

    partitions = malloc(sizeof(partition) * PARTITION_NUM);
    for(int i = 0; i < PARTITION_NUM; i++) {
        partitions[i].content = (node*) malloc(sizeof(node) * NODE_NUM);
        partitions[i].len = 0;
    }
    int queue_size = 10;
    int num_files = 10;
    reducer_ptr rfp = &f_reduce;
    mapper_ptr mfp = &f_map;
     
    //map phase
    thpool * tp = init_thpool(num_mappers, queue_size, 1, num_files);
    char * filename = (char*)malloc(sizeof(char) * 200);
    for(int j = 0; j < num_files; j++) {
        filename = malloc(sizeof(char*) * 200);
        sprintf(filename, "filename%d", j);
        thpool_add_job_map(tp, mfp, filename);
    }
    for(int i = 0; i < num_mappers;  i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    printf("mapping phase finished\n");
    //soring phase
    
    //reduce phase
    thpool * tp2 = init_thpool(num_reducers, queue_size, 0, num_reducers);
    for(int j = 0; j < num_reducers; j++){
        printf("add reducing job %d\n", j);
        argv = malloc(sizeof(char*) * argc);
        for(int i = 0; i < argc; i++) {
            argv[i] = malloc(sizeof(char) * MAX_LENGTH_ARGV);
            // sprintf(argv[i], "come here and reduce %d", i + j);
            strcpy(argv[i], partitions[0].content[0].key);
        }
        thpool_add_job_reduce(tp2, rfp, argc, argv, NULL);
    }
    
    sleep(3);
    printf("main:: after sleep\n");
    for(int i = 0; i < num_reducers;  i++) {
        pthread_join(tp2 -> threads[i], NULL);
    }   
    
    return;    
}

int main(int argc, char * argv[]){
    MR_Run(argc, argv, NULL, 10, NULL, 10, MR_DefaultHashPartition);
    printf("finish all\n");
    return 0;
}
