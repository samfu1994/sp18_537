// Do not modify this file. It will be replaced by the grading scripts
// when checking your project.

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  //printf(1, "%s", "** Placeholder program for grading scripts **\n");
  int i = mprotect(NULL, 0);
  int j = munprotect(NULL, 0);
  printf(1, "%d %d\n", i, j);  
  int * pt = NULL;
  int k = *pt;
  printf(1, "%d\n", k);
  exit();
}
