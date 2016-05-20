#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
    printf(1, "-----------  Welcome to memory testing program! -----------\n\n");
    void* m1;

    int pageSize = 4096;
    m1 = malloc(9*pageSize);
    // m2 = malloc(memToAllocate);
    memset(m1, 'a', 30000);

    getpid();
    if (m1 == 0) {
        printf(1, "couldn't allocate mem?!!\n");
        exit();
    }
    m1 = "This is some text on the memory";

    printf(1, "Free called!\n");
    free(m1);

    // getpid();

    printf(1, "mem ok\n");



    exit();
}
