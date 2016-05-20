#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "-----------  Welcome to memory testing program! -----------\n\n");

  int memToAllocate = 0;
  memToAllocate = 1024;

  getpid();
  void* m1;
  void* m2;
  m1 = malloc(memToAllocate);
  m2 = malloc(memToAllocate);

  getpid();
  if (m1 == 0) {
    printf(1, "couldn't allocate mem?!!\n");
    exit();
  }
  m1 = "This is some text on the memory";
  m2 = "This is some more text on the memory";

  getpid();

  free(m1);
  free(m2);

  getpid();

  printf(1, "mem ok\n");



  exit();
}
