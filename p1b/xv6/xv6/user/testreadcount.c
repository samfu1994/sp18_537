#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char buf[512];
  int i = getreadcount();
  int fd = open(argv[1], 0);
  read(fd, buf, 512);
  int j = getreadcount();
  printf(1, "read count diff is %d\n", j - i );

  exit();
}
