#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

int thread_create(void (*start_routine)(void *, void*), void * arg1, void * arg2)
{
    uint big_enough_size = PGSIZE * 2;
    void * stack = malloc(big_enough_size);
    if(!stack) {
        printf(1, "malloc failed\n");
        exit();
    }
                             
    if((uint)stack % PGSIZE) {
        stack = stack + (PGSIZE - (uint)stack % PGSIZE);
    }
                              
    return clone(start_routine, arg1, arg2, stack);
}
 
 
int thread_join(void * stack) 
{
    free(stack);
    return 0;
}
 

