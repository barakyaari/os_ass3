#include "types.h"
#include "user.h"
#include "mmu.h"


int
main(int argc, char *argv[])
{
    printf(1, "-----------  Welcome to memory testing program! -----------\n\n");
    void* m1;

    int pageSize = 4096;
//    m1 = malloc(11*pageSize);
    m1 = sbrk(12* pageSize);
    printf(1, "Malloced successfuly (m1)!\n");

    memset(m1, 'a', 4096*12);
    printf(1, "Setting m1 finished!\n");

    void* m2 = sbrk(PGSIZE*13);
    printf(1, "Malloced successfuly (m2)!\n");

    memset(m2, 'a', PGSIZE*13);
    printf(1, "Setting m2 finished!\n");

    getpid();
    memset(m1, 'a', 4096*12);
    printf(1, "Setting m1!\n");

    printf(1, "Lots of memsets:\n");

    printf(1, "****************************Memset1:*************************:\n");

    memset(m2, 'a', PGSIZE*13);
    printf(1, "****************************Memset2:*************************:\n");
    memset(m1, 'b', 4096*12);
    printf(1, "****************************Memset3:*************************:\n");
    memset(m2, 'c', PGSIZE*13);
    printf(1, "****************************Memset4:*************************:\n");
    memset(m1, 'd', 4096*12);
    printf(1, "****************************Memset5:*************************:\n");



    // getpid();

    printf(1, "mem ok\n");



    exit();
}
