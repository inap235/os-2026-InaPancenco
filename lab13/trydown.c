// trydown.c: sem_trydown takes a unit if there is one, else returns -1 at once.
#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  sem_init(4, 1);
  printf("value 1: sem_trydown -> %d (took it)\n", sem_trydown(4));
  printf("value 0: sem_trydown -> %d (did not sleep)\n", sem_trydown(4));
  sem_up(4);
  printf("after sem_up: sem_trydown -> %d\n", sem_trydown(4));
  exit(0);
}
