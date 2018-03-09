#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <pthread.h>

#define NAME_MAX_LENGTH 1024
#define PATCH_SIZE 4
#define FILE_SIZE 1024*1024

struct arg_struct {
    char * arr;
    int size;
    int * res_count;
    char * res_char;
    int len;
};

void print_res(struct arg_struct * args, int round) {
    for(int i = 0; i < round; i++) {
        for(int j = 0; j < args[i].len; j++){
            fprintf(stdout, "%d%c", args[i].res_count[j], args[i].res_char[j]);
        }
    }
    return;
}

void concat(int round, struct arg_struct * args) {
    int prev_len;
    for(int i = 1; i < round; i++) {
        prev_len = args[i - 1].len;
        if(args[i - 1].res_char[prev_len - 1] == args[i].res_char[0]) {
            args[i].res_count[0] += args[i - 1].res_count[prev_len - 1];
            args[i - 1].len -= 1;
        }
    }
}

void * process_patch(void * pass_arg) {
    struct arg_struct * arg = (struct arg_struct * ) pass_arg;
    char * arr = arg -> arr;
    arg -> len = 0;
    int size = arg -> size;
    int init = 1;
    char cur, prev;
    int count = 0;
    for(int i = 0; i < size; i++) {
        cur = arr[i];
        if(init || cur == prev) { 
            count += 1;
        }
        else {
            arg -> res_count[arg -> len] = count;
            arg -> res_char[arg -> len] = prev;
            arg -> len += 1;
            count = 1;
        }
        prev = cur;
        init = 0;
    }
    if(count != 0) {
        arg -> res_count[arg -> len] = count;
        arg -> res_char[arg -> len] = prev;
        arg -> len += 1;
    }
    return NULL;
}

void zip_file(int fd, int cur_size) {
    if(cur_size == 0) return;

    const int offset = 0;
    int num_thread = get_nprocs();
    pthread_t ** threads = malloc(sizeof(pthread_t * ) * num_thread);
    for(int i = 0; i < num_thread; i++) {
        threads[i] = (pthread_t *)malloc(sizeof(pthread_t));
    } 
    char * mmaped_data = mmap(NULL, cur_size, PROT_READ, MAP_PRIVATE, fd, offset);
    if(mmaped_data == MAP_FAILED) {
        fprintf(stderr, "error when mmap file\n");
        exit(1);
    }
    
    int start;
    char * start_addr;
    int cur_patch_size;
    
    int round = (cur_size - 1) / PATCH_SIZE + 1;
    int patch_size = PATCH_SIZE;
    if(round > num_thread) {
        round = num_thread;
        patch_size = cur_size / round;
    } 
        
    struct arg_struct * pass_arg = (struct arg_struct *) malloc(sizeof(struct arg_struct) * round);
    for(int i = 0; i < round; i++) {
        start = i * patch_size;
        if(start > cur_size) {
            break;
        }
        cur_patch_size = patch_size;
        start_addr = &mmaped_data[start];  
        if(start + patch_size > cur_size) {
            cur_patch_size = cur_size - start;
        }
        pass_arg[i].size = cur_patch_size;
        pass_arg[i].arr = start_addr;
        pass_arg[i].res_char = (char*) malloc(sizeof(char) * PATCH_SIZE);
        pass_arg[i].res_count = (int*) malloc(sizeof(int) * PATCH_SIZE);
        pass_arg[i].len = -1;
        if(pthread_create(threads[i], NULL, &process_patch, (void*) &pass_arg[i]) != 0) {
            fprintf(stderr, "error when creating thread\n");
            exit(1);
        }
    }

    for(int i = 0; i < round; i++) {
        if(pthread_join(*threads[i], NULL) != 0) {
            fprintf(stderr, "error when pthread_join\n");
        }
    }
    
    if(round > 1) {
        concat(round, pass_arg);
    }
    print_res(pass_arg, round);
    
    for(int i = 0; i < round; i++) {
        free(pass_arg[i].res_char);
        free(pass_arg[i].res_count);
    }
    free(pass_arg);

    for(int i = 0; i < num_thread; i++) {
        free(threads[i]);
    }
    free(threads);

    if(munmap(mmaped_data, cur_size) != 0) {
        fprintf(stderr, "error when unmmap\n");
        exit(1);
    }
    return;
}

int main(int argc, char * argv []) {
	if(argc == 1) {
		printf("pzip: file1 [file2 ...]\n");
		exit(1);
	}
	int num_file = argc - 1;
    int num_thread = get_nprocs();
	int * fds = malloc(sizeof(int) * num_file);
    int * sizes = malloc(sizeof(int) * num_file);
    //pthread_t ** threads = malloc(sizeof(pthread_t * ) * num_thread);

    for(int i = 0; i < num_thread; i++) {
        //pthread_create(threads[i], NULL,);
    }
    
    for(int i = 0; i < num_file; i++) {
        struct stat st;
        fds[i] = open(argv[i + 1], O_RDWR);
        fstat(fds[i], &st);
        sizes[i] = st.st_size;    
    }

    for(int i = 0; i < num_file; i++) {
        zip_file(fds[i], sizes[i]);
    }
    free(fds);
    free(sizes);
	return 0;
}
