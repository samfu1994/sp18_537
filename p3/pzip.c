#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

#define NAME_MAX_LENGTH 1024
#define PATCH_SIZE 4
#define MAX_COUNT 1024*1024*1024

struct arg_struct {
    char * arr;
    int size;
    int * res_count;
    char * res_char;
    int len;
};

void print_res(struct arg_struct * args, int round, int last_file, int first_file, char prev_file_end_char, int prev_file_end_count) {
    int enter = 0;
    int cur = 0;
    char * res = malloc(sizeof(char) * MAX_COUNT * 2);
    for(int i = 0; i < round; i++) {
        for(int j = 0; j < args[i].len; j++){
            if(!first_file && enter == 0) {
                if(prev_file_end_char == args[i].res_char[j]) {
                    args[i].res_count[j] += prev_file_end_count;
                }
                else {
                    memcpy(res + cur, &prev_file_end_count, sizeof(int));
                    cur += sizeof(int);
                    memcpy(res + cur, &prev_file_end_char, sizeof(char));
                    cur += sizeof(char);
                }
                enter = 1;
            }
            if(!last_file && i == round -1 && j == args[i].len - 1) break;
            memcpy(res + cur, &args[i].res_count[j], sizeof(int));
            cur += sizeof(int);
            memcpy(res + cur, &args[i].res_char[j], sizeof(char));
            cur += sizeof(char);
        }
    }
    fwrite((const void *) res, cur, 1, stdout);
    fflush(stdout);
    free(res);
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

void zip_file(int fd, int cur_size, int* last_file,  int * first_file, char * prev_file_end_char, int * prev_file_end_count) {
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
        else if(i == round - 1 && start + patch_size != cur_size) {
            cur_patch_size = cur_size - start;
        }
        //fprintf(stdout, "i is %d, size is %d, start is %d, total is %d\n", i, cur_patch_size, start, cur_size);
        pass_arg[i].size = cur_patch_size;
        pass_arg[i].arr = start_addr;
        pass_arg[i].res_char = (char*) malloc(sizeof(char) * MAX_COUNT);
        pass_arg[i].res_count = (int*) malloc(sizeof(int) * MAX_COUNT);
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
    print_res(pass_arg, round, *last_file, *first_file, *prev_file_end_char, *prev_file_end_count);
    *prev_file_end_char = pass_arg[round-1].res_char[pass_arg[round-1].len - 1];
    *prev_file_end_count = pass_arg[round-1].res_count[pass_arg[round-1].len - 1];

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
    
    *first_file = 0;
    return;
}

int main(int argc, char * argv []) {
	if(argc == 1) {
		printf("pzip: file1 [file2 ...]\n");
		exit(1);
	}
	int num_file = argc - 1;
	int fd;
    int size;
    int first = 1;
    int last = 0;
    char end_char;
    int end_count;
    for(int i = 0; i < num_file; i++) {
        struct stat st;
        fd = open(argv[i + 1], O_RDONLY);
        fstat(fd, &st);
        size = st.st_size;
        if(i == num_file - 1) {
            last = 1;
        }
        zip_file(fd, size, &last, &first, &end_char, &end_count);
        if(fd < 0)
            fprintf(stderr, "error when open %d\n", fd);
        close(fd);
    }

	return 0;
}
