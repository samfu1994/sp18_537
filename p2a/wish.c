#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define PATH_NUM 128
#define PATH_LENGTH 128
#define PARA_NUM 64
char ** paths;
int path_count;

void batchMode(char * argv) {
    return;
}

void user_exit() {
    printf("exit now\n");
    exit(0);
}

void user_cd(char ** cmd_tokens) {
    char * dir = cmd_tokens[1];
    int err = chdir(dir);
    if(err) {
        printf("cannot change to dir %s, error code %d\n", dir, err);
    }
    return;
}

void user_path(char ** cmd_tokens) {
    int cur = 1;
    while(cmd_tokens[cur] != NULL) {
        strcpy(paths[path_count], cmd_tokens[cur]);
        path_count += 1;
        cur += 1;
    }
    return;
}

void print_path() {
    printf("path now is \n");
    for(int i = 0; i < path_count; i++) {
        printf("%s\n", paths[i]);
    }
    return;
}

int is_builtin_cmd(char * cmd) {
        if(!strcmp(cmd, "pp") || !strcmp(cmd, "exit") || !strcmp(cmd, "cd") || !strcmp(cmd, "path")) return 1;
        return 0;
}

char ** parse_cmd(char * cmd, int * count) {
    char ** argv = (char**) malloc(sizeof(char*) * PARA_NUM);
    char * cur = (char*) malloc(sizeof(char) * 256);
    cur = strtok(cmd, " \n");
    *count = 0;
    while(cur) {
        //strcpy(argv[*count], cur);
        argv[*count] = cur;
        *count += 1;
        cur = strtok(NULL, " \n");
    }
    return argv;
}
void my_exec(char ** my_argv) {
    int full_len = 256;
    char * full_path = (char*) malloc(sizeof(char) * full_len);
    int found = 0;
    for(int i = 0; i < path_count; i++) {
        memset(full_path, 0, full_len);
        strcpy(full_path, paths[i]);
        strcat(full_path, "/");
        strcat(full_path, my_argv[0]);
        if(!access(full_path, X_OK)) {
            found = 1;
            //printf("executing %s...\n", full_path);
            execv(full_path, my_argv);
        }
    }
    if(!found) {
        printf("cannot find executable file %s\n", my_argv[0]);
    }
}

void run(char ** cmd_tokens, int argv_count) {
        //printf("running executable...\n");
        char * redirect = ">";
        char * parallel = "&";
        char ** my_argv = (char**) malloc(sizeof(char*) * PARA_NUM);
        int cur = 0;
        int base = 0;
        while(base + cur < argv_count) {
            printf("%d, %d, %d\n", base, cur, argv_count);
            if(strcmp(cmd_tokens[base + cur], redirect) == 0) {
                //get next token which is file name.
                int fd = open(cmd_tokens[base + cur + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                dup2(fd, 1);   // make stdout go to file
                dup2(fd, 2);
                cur += 2;
            }
            else if(strcmp(cmd_tokens[base + cur], parallel) == 0) {
                //exec the current one
                pid_t pid = fork();
                if(pid == -1) {
                    perror("fork failed");
                    exit(1);
                }
                else if(pid == 0) {
                    my_exec(my_argv);
                    exit(2);
                }
                else {
                    base += cur + 1;
                    cur = 0;
                }
            }
            else{
                //store the current argv to a particular array
                my_argv[cur] = cmd_tokens[base + cur];
                cur += 1;
            }

        }
        pid_t pid_last = fork();
        if(pid_last == -1) {
            perror("fork failed");
            exit(1);
        }
        else if(pid_last == 0) {
            my_exec(my_argv);
            exit(2);
        }
        else {
            //dup2(stdout, 1);
            //dup2(stderr, 2);
            return;
        }
}

int main(int argc, char * argv[]) {
    if(argc == 2) {
        batchMode(argv[1]);
    }
    paths = (char**) malloc(sizeof(char*) * PATH_NUM);
    for(int i = 0; i < PATH_NUM; i++){
        paths[i] = (char*) malloc(sizeof(char) * PATH_LENGTH);
    }
    paths[0] = "/bin";
    size_t len = 256;
    path_count = 1;
    int *argv_count = (int*) malloc(sizeof(int));
    char * line = (char*) malloc(sizeof(char) * len);
    while(1) {
        printf("wish> ");
        if(getline(&line, &len, stdin) == -1) {
            printf("error reading command\n");
            break;
        }

        char ** cmd_tokens = parse_cmd(line, argv_count); 
        if(is_builtin_cmd(cmd_tokens[0])) {
            if(!strcmp(cmd_tokens[0], "exit")) {
                user_exit();
            }else if(!strcmp(cmd_tokens[0], "cd")) {
                user_cd(cmd_tokens);
            }else if(!strcmp(cmd_tokens[0], "path")) {
                user_path(cmd_tokens);
            }else if(!strcmp(cmd_tokens[0], "pp")) {
                print_path();
            }
        }
        else {
            run(cmd_tokens, *argv_count);
        }
    }
    return 0;
}
