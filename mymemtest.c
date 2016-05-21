#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
    printf(1, "-----------  Welcome to memory testing program! -----------\n\n");
    void* m1;

    int pageSize = 4096;
//    m1 = malloc(11*pageSize);
    m1 = sbrk(12* pageSize);
    printf(1, "Malloced successfuly!\n");
    memset(m1, 'a', 4096*12);
    void* m2 = sbrk(4096);
    memset(m2, 'a', 4096);
    getpid();
    memset(m1, 'a', 4096*12);

    printf(1, "getpid successfuly!\n");


    int pid = fork();
    if(pid !=0)
        wait();

    // getpid();

    printf(1, "mem ok\n");



    exit();
}
