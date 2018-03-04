#include "types.h"
#include "pstat.h"
#include "user.h"

int main(int argc, char * argv[]) {
    int pid = getpid();
    int root_pid = getpid();
    struct pstat * p = (struct pstat*) malloc(sizeof(struct pstat));;
    getpinfo(p);

    for(int i = 0; i < 3;i++) {
        if(pid == root_pid){
            fork();
        }
        pid = getpid();
        if(pid != root_pid) {
            printf(1, "child is %d\n", pid);
            exit();
        }
    }
    exit();
    
    /*struct pstat * p = (struct pstat*) malloc(sizeof(struct pstat));;
    getpinfo(p);
    int pid = getpid();
    printf(1, "my pid is %d\n", pid);
    settickets(15);
    getpinfo(p);
    for(int i = 0; i < 64; i++) {
            printf(1, "%d,  %d,  %d,  %d\n", p -> inuse[i], p -> tickets[i], p -> pid[i], p -> ticks[i]);
    }
    printf(1, "-------------------\n");
    */
    //settickets(20);
    //getpinfo(NULL);
    /*for(int i = 0; i < 64; i++) {
            printf(1, "%d,  %d,  %d,  %d\n", p -> inuse[i], p -> tickets[i], p -> pid[i], p -> ticks[i]);
    }
    */

}
