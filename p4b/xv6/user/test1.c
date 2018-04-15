#include "types.h"
#include "stat.h"
#include "user.h"

char buf[512];

void func(void * a, void * b) {
    int * p1 = a;
    int * p2 = b;
    printf(1, "child process: a is %d, b is %d\n", *p1, *p2);
    //return ;
    exit() ;
}

int
main(int argc, char *argv[])
{
  int a = 222, b = 333;
  printf(1, "clone is %d\n", thread_create(&func, (void*)&a, (void*) &b)); 
  printf(1, "after create\n");
  int pid = thread_join();
  printf(1, "after join pid : %d\n", pid);
  //printf(1, " clone is %d", clone(&func, (void*) & fd, (void*) &i, (void*) & stack));
  /*
   * if(argc <= 1){
    cat(0);
    exit();
  }

  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "cat: cannot open %s\n", argv[i]);
      exit();
    }
    cat(fd);
    close(fd);
  }
  */
  exit();
}
