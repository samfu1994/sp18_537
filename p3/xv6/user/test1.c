// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"
#define PGSIZE 4096
int
main(int argc, char *argv[])
{
  //printf(1, "%s", "** Placeholder program for grading scripts **\n");
    int n = 1;
    char a[PGSIZE * n];
    a[0] = 'k';
    a[1] = 'o';
    mprotect((void*)a, n);
    //munprotect((void*)a, n);
    //a[2] = 'o';
    //a[3] = 'k';
    printf(1, "%c%c%c%c\n", a[0], a[1], a[2], a[3]);
    exit();
}
