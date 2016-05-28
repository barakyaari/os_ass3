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
//    printf(1, "Setting m1 finished!\n");
    printf(1, "Trying second sbark:\n");

    void* m2 = sbrk(PGSIZE*12);
    printf(1, "Setting memory to 'a':\n");

    memset(m2, 'a', PGSIZE*3);
    printf(1, "finished setting memory to 'a':\n");



//
//    int pid=fork();
//    if(pid == 0) {
//        printf(1, "Child alive!");
//    }
//    else {
//        wait();
//        printf(1, "Parent alive!");
//    }

    printf(1, "mem ok\n");

    sbrk(24*pageSize*(-1));
    printf(1, "freex ok\n");

    exit();
}
