#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define PGSIZE 4096

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int
thread_create(void (*start_routine)(void *, void*), void * arg1, void * arg2)
{
    uint big_enough_size = PGSIZE * 2;
    void * stack = malloc(big_enough_size);
    if(!stack) {
        printf(1, "malloc failed\n");
        return -1;
    }
    
    if((uint)stack % PGSIZE) {
        stack = stack + (PGSIZE - (uint)stack % PGSIZE);
    }

    return clone(start_routine, arg1, arg2, stack);
}

static inline int fetch_and_add(volatile int* variable, int value)
{
      __asm__ volatile("lock; xaddl %0, %1"
        : "+r" (value), "+m" (*variable) // input+output
        : // No input-only
        : "memory"
      );
      return value;
}

int thread_join() 
{
    void * ustack;
    int id = join(&ustack);
    if(id != -1) {
        free(ustack);
    }
    return id;
}

void lock_init(volatile lock_t *lock) {
  lock -> ticket = 0;
  lock -> turn = 0;
}

void lock_acquire(volatile lock_t *lock) {
  int my_turn = fetch_and_add(&(lock -> ticket), 1);
  while(lock -> turn != my_turn)
    ; // spin

}

void lock_release(volatile lock_t *lock) {
  lock -> turn = lock -> turn + 1;
}

