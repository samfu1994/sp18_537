#include "thpool.h"

#define TEST_NUM 16
unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

void MR_Emit(char *key, char *value){
    
}


void MR_Run(int argc, char *argv[], 
        Mapper map, int num_mappers, 
        Reducer reduce, int num_reducers, 
        Partitioner partition){

    int queue_size = 10;
    int num_files = 10;
    reducer_ptr rfp = &f_reduce;
    mapper_ptr mfp = &f_map;
     
    //map phase
    thpool * tp = init_thpool(num_mappers, queue_size, 1, num_files);
    char * filename = (char*)malloc(sizeof(char) * 200);
    for(int j = 0; j < num_files; j++) {
        printf("adding map job%d\n", j);
        sprintf(filename, "filename%d", j);
        thpool_add_job_map(tp, mfp, filename);
    }
    //soring phase
    /*
    //reduce phase
    thpool * tp2 = init_thpool(num_reducers, queue_size, 0, num_reducers);
    for(int j = 0; j < TEST_NUM; j++){
        printf("add reducing job %d\n", j);
        argv = malloc(sizeof(char*) * argc);
        for(int i = 0; i < argc; i++) {
            argv[i] = malloc(sizeof(char) * MAX_LENGTH_ARGV);
            sprintf(argv[i], "%d", i + j);
        }
        thpool_add_job_reduce(tp2, rfp, argc, argv, NULL);
    }*/
    
    sleep(3);
    printf("main:: after sleep\n");
    for(int i = 0; i < num_mappers;  i++) {
        pthread_join(tp -> threads[i], NULL);
    }
    
    return;    
}

int main(int argc, char * argv[]){
    MR_Run(argc, argv, NULL, 10, NULL, 10, MR_DefaultHashPartition);
    printf("finish all\n");
    return 0;
}
