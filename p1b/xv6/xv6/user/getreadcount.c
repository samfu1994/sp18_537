#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int i = getreadcount();
  printf(1, "read count now is %d\n", i );

  exit();
}
