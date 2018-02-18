#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#define PATH_NUM 128
#define PATH_LENGTH 128
#define PARA_NUM 64
#define PARA_LENGTH 64
#define LINE_LENGTH 256
char ** paths;
int path_count;
int batch_mode;

void batchMode(char * argv) {
    return;
}

void print_error(int code) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
    exit(code);
}

void user_exit(int *argv_count) {
    if(*argv_count != 1) {
        print_error(0);
        return;
    }
    free(argv_count);
    exit(0);
}

void user_cd(char ** cmd_tokens, int argv_count) {
    if(argv_count != 2) {
        print_error(0);
        return;
    }
    char * dir = cmd_tokens[1];
    int err = chdir(dir);
    if(err) {
        print_error(0);
    }
    return;
}

void user_path(char ** cmd_tokens, int argv_count) {
    path_count = 0;
    while(path_count < argv_count - 1) {
        memset(paths[path_count], 0, PATH_LENGTH);
        strcpy(paths[path_count], cmd_tokens[path_count + 1]);
        path_count += 1;
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
    for(int i = 0; i < PARA_NUM; i++) {
        argv[i] = (char*) malloc(sizeof(char) * PARA_LENGTH);
    }
    char * format_line = (char*) malloc(sizeof(char) * LINE_LENGTH);
    int i = 0, j = 0;
    while(j < LINE_LENGTH && cmd[i] != '\n' && cmd[i] != 0) {
        char c = cmd[i];
        if(c == '>' || c == '&') {
            format_line[j++] = ' ';
            format_line[j++] = c;
            format_line[j++] = ' ';
        }
        else{
            format_line[j++] = c;
        }
        i++;
    }
    char * cur = strtok(format_line, " \n");
    *count = 0;
    while(cur) {
        strcpy(argv[*count], cur);
        *count += 1;
        cur = strtok(NULL, " \n");
    }
    free(format_line);
    return argv;
}

void clear_argv(char ** my_argv) {
    my_argv[0] = 0;
    return;
}

void my_exec(char ** my_argv, int fd, int is_redirect) {
    if(my_argv[0] == 0) return;
    int full_len = 256;
    if(is_redirect) {
        dup2(fd, 1);
        dup2(fd, 2);
    }
    char * full_path = (char*) malloc(sizeof(char) * full_len);
    int found = 0;
    for(int i = 0; i < path_count; i++) {
        memset(full_path, 0, full_len);
        strcpy(full_path, paths[i]);
        strcat(full_path, "/");
        strcat(full_path, my_argv[0]);
        if(!access(full_path, X_OK)) {
            found = 1;
            free(full_path);
            execv(full_path, my_argv);
        }
    }
    free(full_path);
    if(!found) {
        print_error(0);
    }
}

void run(char ** cmd_tokens, int argv_count) { 
        char * redirect = ">";
        char * parallel = "&";
        char ** my_argv = (char**) malloc(sizeof(char*) * PARA_NUM);
        memset(my_argv, 0, sizeof(char*) * PARA_NUM);
        int cur = 0;
        int base = 0;
        int status;
        int is_redirect = 0;
        int fd;
        while(base + cur < argv_count) {
            if(strcmp(cmd_tokens[base + cur], redirect) == 0) {
                //check whether there is a cmd before > 
                if(cur == 0) {
                    print_error(0);
                }

                //check whether a dest file exist in cmd
                if(base + cur + 1 == argv_count || strcmp(cmd_tokens[base+cur+1], parallel) == 0) {
                    print_error(0);
                }

                //check whether multiple redirect symbol exist
                if(strcmp(cmd_tokens[base+cur+1], redirect) == 0) {
                    print_error(0);
                }
                
                //check whether multiple dest files exist
                if(base + cur + 2 != argv_count && strcmp(cmd_tokens[base+cur+2], parallel) != 0) {
                    print_error(0);
                }

                fd = open(cmd_tokens[base + cur + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                is_redirect = 1;
                cur += 2;
            }
            else if(strcmp(cmd_tokens[base + cur], parallel) == 0) {
                //exec the current one
                pid_t pid = fork();
                if(pid == -1) {
                    print_error(0);
                    exit(1);
                }
                else if(pid == 0) {
                    my_exec(my_argv, fd, is_redirect);
                    exit(1);
                }
                else {
                    is_redirect = 0;
                    base += cur + 1;
                    cur = 0;
                    clear_argv(my_argv);
                    waitpid(pid, &status, WUNTRACED);
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
            print_error(0);
            exit(1);
        }
        else if(pid_last == 0) {
            my_exec(my_argv, fd, is_redirect);
            exit(1);
        }
        else {
            is_redirect = 0;
            waitpid(pid_last, &status, WUNTRACED);
            free(my_argv);
            return;
        }
}

int main(int argc, char * argv[]) {
    paths = (char**) malloc(sizeof(char*) * PATH_NUM);
    for(int i = 0; i < PATH_NUM; i++){
        paths[i] = (char*) malloc(sizeof(char) * PATH_LENGTH);
    }
    strcpy(paths[0], "/bin");
    size_t len = 256;
    path_count = 1;
    int *argv_count = (int*) malloc(sizeof(int));
    char * line = (char*) malloc(sizeof(char) * len);
    FILE * file;
   
    if(argc >= 2) {    
        if(argc > 2){
            print_error(1);
        }
        batch_mode = 1;
        file = fopen(argv[1], "r");
        if(NULL == file){
            print_error(1);
        }
    }
    
    while(1) {
        if(!batch_mode){
            printf("wish> ");
            fflush(stdout);
            if(getline(&line, &len, stdin) == -1) {
                break;
            }
        }
        else{
            if(getline(&line, &len, file) == -1) {
                break;
            }
        }

        char ** cmd_tokens = parse_cmd(line, argv_count); 

        if(*argv_count == 0) continue;
        if(is_builtin_cmd(cmd_tokens[0])) {
            if(!strcmp(cmd_tokens[0], "exit")) {
                for(int i = 0; i < PATH_NUM; i++){
                    free(paths[i]);
                }                
                free(paths);
                for(int i = 0; i < PARA_NUM; i++) {
                    free(cmd_tokens[i]);
                }
                free(cmd_tokens);
                free(line);
                user_exit(argv_count);
            }else if(!strcmp(cmd_tokens[0], "cd")) {
                user_cd(cmd_tokens, *argv_count);
            }else if(!strcmp(cmd_tokens[0], "path")) {
                user_path(cmd_tokens, *argv_count);
            }else if(!strcmp(cmd_tokens[0], "pp")) {
                print_path();
            }else{
                print_error(0);
            }
        }
        else {
            run(cmd_tokens, *argv_count);
        }
    }
    return 0;
}
