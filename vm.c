#include <wchar.h>
#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

#define MAX_PSYC_PAGES 15
#define MAX_TOTAL_PAGES 30

extern int createSwapFile(struct proc* p);
extern int removeSwapFile(struct proc* p);



#ifdef NONE
int defaultPolicy = 1;

#else
int defaultPolicy = 0;

#endif


char buff[PGSIZE];

int lastPageSwappedOut = 3;
uint findPageInFile(int num);

void updateStatsOnPageEntrance(int pageNum);

extern char data[];  // defined by kernel.ld

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
    pde_t *pde;
    pte_t *pgtab;

    pde = &pgdir[PDX(va)];
    if (*pde & PTE_P) {
        pgtab = (pte_t*)p2v(PTE_ADDR(*pde));
    } else {
        if (!alloc || (pgtab = (pte_t*)kalloc()) == 0)
            return 0;
        // Make sure all those PTE_P bits are zero.
        memset(pgtab, 0, PGSIZE);
        // The permissions here are overly generous, but they can
        // be further restricted by the permissions in the page table
        // entries, if necessary.
        *pde = v2p(pgtab) | PTE_P | PTE_W | PTE_U;
    }
    return &pgtab[PTX(va)];
}

pde_t *kpgdir;  // for use in scheduler()
struct segdesc gdt[NSEGS];

void printStats(struct proc *p){
    if(!p){
        return;
    }
    int i;
    cprintf("  Num\t|  P\t|  R\t|  SO\n");

    cprintf("----------------------------------------------\n");

    for(i = 0; i < 30; i++){
        pte_t *pte;
        if ((pte = walkpgdir(p->pgdir, (void *) (i*PGSIZE), 0)) == 0)
            panic("copyuvm: pte should exist");
        int referenced = (*pte & PTE_A);
        int present = (*pte & PTE_P);
        int swappedOut = (*pte & PTE_PG);

        cprintf("  %d\t|  %d\t|  %d\t|  %d\t \n",i, present, referenced, swappedOut);

    }
}

void clearReferencedBits() {
    int i;

    for (i = 3; i < 30; i++) {
        pte_t *pte;
        if ((pte = walkpgdir(proc->pgdir, (void *) (i * PGSIZE), 0)) == 0) {
            panic("copyuvm: pte should exist");
        }
        *pte &= (~PTE_A);
    }

}

void *selectPageToMoveFifo() {
    int i;
    int currentMax = -1;
    int selectedPage = -1;
    for (i = 3; i < 15; i++) {
        if (proc->fileData->arrivalTime[i] > currentMax) {
            currentMax = proc->fileData->arrivalTime[i];
            selectedPage = i;
        }
    }
    proc->fileData->arrivalTime[selectedPage] = 0;

    return (void*)(selectedPage * PGSIZE);
}

void *selectPageToMoveScFifo() {
    int i;
    int currentMax = -1;
    int selectedPage = -1;
    while(selectedPage == -1) {
        for (i = 3; i < 30; i++) {//Find the Page with the highest arrival time:
            if (proc->fileData->arrivalTime[i] > currentMax) {
                currentMax = proc->fileData->arrivalTime[i];
                selectedPage = i;
            }
        }
        //Clear referenced bit and continue if needed:
        //Check if page is referenced:
        pte_t *pte;
        if ((pte = walkpgdir(proc->pgdir, (void *) (selectedPage*PGSIZE), 0)) == 0)
            panic("copyuvm: pte should exist");
        int referenced = (*pte & PTE_A);
        if(referenced) {
            //Clear referenced bit:
            *pte &= (~PTE_A);
            //Put min value to arrival time and update the time for all:
            updateStatsOnPageEntrance(selectedPage);
            selectedPage = -1;
            currentMax = -1;
        }
        else{
            break;
        }
    }
    proc->fileData->arrivalTime[selectedPage] = 0;

    return (void*)(selectedPage * PGSIZE);
}

void *selectPageToMoveNfu() {
    int i;
    int currentMin = 2147483647;
    int selectedPageIndex = -1;
    for (i = 3; i < 30; i++) {
        pte_t *pte;
        //Check if page is present:
        if ((pte = walkpgdir(proc->pgdir, (void *) ((int)i*PGSIZE), 0)) == 0)
            panic("copyuvm: pte should exist");
        int present = (*pte & PTE_P);
        if (present && proc->fileData->nfuCounter[i] <= currentMin) {
            currentMin = proc->fileData->nfuCounter[i];
            selectedPageIndex = i;
        }
    }
    proc->fileData->arrivalTime[selectedPageIndex] = 0xFFFF;
    return (void*)(selectedPageIndex * PGSIZE);
}


//
//    for (i = lastPageSwappedOut*PGSIZE; i < proc->numOfPages*PGSIZE; i += PGSIZE) {
//
//        cprintf("-- checking page:  %d --\n", i/PGSIZE);
//        if ((pte = walkpgdir(proc->pgdir, (void *) i, 0)) == 0)
//            panic("copyuvm: pte should exist");
//
//        int present = (*pte & PTE_P);
//        int swappedOut = (*pte & PTE_PG);
//
//
//        if(present && !swappedOut){ //Swapping out the page:
//            lastPageSwappedOut = (i/PGSIZE % 12) + 3;
//            return (void*)i;
//        }
//    }
//    panic("No page found for swapping");
//}

void cleanProcessPagesData(){
    removeSwapFile(proc);
//    int i;
//    for (i = 0; i < 30; i++) {
//        pte_t *pte;
//        if ((pte = walkpgdir(proc->pgdir, (void *) (i * PGSIZE), 0)) == 0) {
//            panic("cleanProcessPagesData: pte should exist");
//        }
//        int present = (*pte & PTE_P);
//        if(present){
//            uint pa = PTE_ADDR(*pte);
//            kfree(p2v(pa));
//        }
//    }

}

void *selectPageToMove() {
#ifdef NONE
    return selectPageToMoveFifo();


#elif FIFO
    return selectPageToMoveFifo();

#elif SCFIFO
    return selectPageToMoveScFifo();

#elif NFU
    return selectPageToMoveNfu();

#endif

}

void movePage(pte_t* pte) {
    char buff[PGSIZE];
    safestrcpy(buff, (char*)pte, PGSIZE);
    int i;
    for(i = 0; i < 15; i++){
        if(proc->fileData->swapFileMapping[i] == -1){
            break;
        }
    }
    if(writeToSwapFile(proc, buff, i*PGSIZE, PGSIZE) < 4096){
        panic("WriteToSwapFile Failed.");
    }
    proc->fileData->swapFileMapping[i] = (int)pte/PGSIZE;


    proc->fileData->lastIndex = proc->fileData->lastIndex + PGSIZE;

    cprintf("SwapFileMapping:\n");
    for(i = 0; i < 15; i++){
        cprintf("[%d->%d]\n", i, proc->fileData->swapFileMapping[i]);
    }
    cprintf("\n");
}

void loadPageFromSwapFile(char* buff, int pageNumber){
    cprintf("loading page from swapfile, Number:%d\n", pageNumber);
    int i;
    cprintf("Moving from buff to pte:\n");
    for(i = 0; i < 15; i++){
        if(proc->fileData->swapFileMapping[i] == pageNumber){
            break;
        }
    }
    int locationInSwapFile = i * PGSIZE;
    proc->fileData->swapFileMapping[i] = -1;
    readFromSwapFile(proc, buff, locationInSwapFile, PGSIZE);
}

uint findPageInFile(int pageNum) {
    return pageNum * PGSIZE;
}


// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
    struct cpu *c;

    // Map "logical" addresses to virtual addresses using identity map.
    // Cannot share a CODE descriptor for both kernel and user
    // because it would have to have DPL_USR, but the CPU forbids
    // an interrupt from CPL=0 to DPL=3.
    c = &cpus[cpunum()];
    c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, 0);
    c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
    c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
    c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

    // Map cpu, and curproc
    c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);

    lgdt(c->gdt, sizeof(c->gdt));
    loadgs(SEG_KCPU << 3);

    // Initialize cpu-local storage.
    cpu = c;
    proc = 0;
}



// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
    char *a, *last;
    pte_t *pte;
    a = (char*)PGROUNDDOWN((uint)va);
    last = (char*)PGROUNDDOWN(((uint)va) + size - 1);

    for (;;) {
        if ((pte = walkpgdir(pgdir, a, 1)) == 0) {
            return -1;
        }
        if ((*pte & PTE_P) & !(*pte & PTE_PG)) {
            panic("remap");
        }
        *pte = pa | perm | PTE_P;


        if (a == last)
            break;
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

void* selectOnePageToRemove() {
    void *va = selectPageToMove();//Returns index * PGSize
    pte_t* pte = walkpgdir(proc->pgdir, va , 0);
//    uint pa = PTE_ADDR(*pte);
//    kfree(p2v(pa));
    *pte = (*pte | PTE_PG);//Turn on in hard disk flag
    *pte = (*pte & ~PTE_P);//turn off present flag
    return va;
}


// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
    void *virt;
    uint phys_start;
    uint phys_end;
    int perm;
} kmap[] = {
        { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
        { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
        { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
        { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
    pde_t *pgdir;
    struct kmap *k;

    if ((pgdir = (pde_t*)kalloc()) == 0)
        return 0;

    memset(pgdir, 0, PGSIZE);

    if (p2v(PHYSTOP) > (void*)DEVSPACE)
        panic("PHYSTOP too high");


    for (k = kmap; k < &kmap[NELEM(kmap)]; k++){

        if (mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                     (uint) k->phys_start, k->perm) < 0) {
            return 0;
        }
    }
    return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
    kpgdir = setupkvm();
    switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
    lcr3(v2p(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
    pushcli();
    cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts) - 1, 0);
    cpu->gdt[SEG_TSS].s = 0;
    cpu->ts.ss0 = SEG_KDATA << 3;
    cpu->ts.esp0 = (uint)proc->kstack + KSTACKSIZE;
    ltr(SEG_TSS << 3);
    if (p->pgdir == 0)
        panic("switchuvm: no pgdir");
    lcr3(v2p(p->pgdir));  // switch to new address space
    popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
    char *mem;

    if (sz >= PGSIZE)
        panic("inituvm: more than a page");
    mem = kalloc();
    memset(mem, 0, PGSIZE);
    mappages(pgdir, 0, PGSIZE, v2p(mem), PTE_W | PTE_U);
    memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
    uint i, pa, n;
    pte_t *pte;

    if ((uint) addr % PGSIZE != 0)
        panic("loaduvm: addr must be page aligned");
    for (i = 0; i < sz; i += PGSIZE) {
        if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
            panic("loaduvm: address should exist");
        pa = PTE_ADDR(*pte);
        if (sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        if (readi(ip, p2v(pa), offset + i, n) != n)
            return -1;
    }
    return 0;
}


void savePageInSwapFile(char* buff, int pageNumber){
    cprintf("Saving page (%d) in swap file\n", pageNumber);
    int i;
    //Find free slot:
    for(i = 0; i < 15; i++){
        if(proc->fileData->swapFileMapping[i] == -1){
            break;
        }
    }

    if(writeToSwapFile(proc, buff, i*PGSIZE, PGSIZE) < 4096){
        panic("WriteToSwapFile Failed.");
    }
    proc->fileData->swapFileMapping[i] = pageNumber;
    cprintf("SwapFileMapping:\n");
    for(i = 0; i < 15; i++){
        cprintf("[%d->%d]\n", i, proc->fileData->swapFileMapping[i]);
    }
    cprintf("\n");
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
    cprintf("allocuvm starting, pid is: %d\n", proc->pid);
    char *mem;
    uint a;

    if (newsz >= KERNBASE)
        return 0;
    if (newsz < oldsz)
        return oldsz;
    if(newsz > PGSIZE * MAX_TOTAL_PAGES){
        panic("Process's page number exceeded.");
    }

    a = PGROUNDUP(oldsz);
    for (; a < newsz; a += PGSIZE) {

        mem = kalloc();

        if (mem == 0) {
            cprintf("allocuvm out of memory\n");
            deallocuvm(pgdir, newsz, oldsz);
            return 0;
        }
        memset(mem, 0, PGSIZE);

        if(proc && !defaultPolicy && proc->pid > 2) {
            int pageNumber = a/PGSIZE;
            cprintf("Adding page Number %d, pid=%d\n", pageNumber, proc->pid);
            if (proc->numOfPages >= MAX_PSYC_PAGES) {//More than 15 pages:

                void* pageToRemove = selectOnePageToRemove();
                cprintf("Found page to remove: %x\n", pageToRemove);
                memmove(buff, pageToRemove, PGSIZE);
                //TODO:THis causes looking for page 3 again and again....
                //Probably mapping it incorrectly.
                cprintf("memmove finished\n");

                savePageInSwapFile(buff, (int)a/PGSIZE);
                mappages(pgdir, (char *) a, PGSIZE, v2p(pageToRemove), PTE_W | PTE_U);
            }

            updateStatsOnPageEntrance((int)a/PGSIZE);
            mappages(pgdir, (char *) a, PGSIZE, v2p(mem), PTE_W | PTE_U);
            pte_t *pte;
            if ((pte = walkpgdir(pgdir, (void*)a, 0)) == 0) {
                panic("loaduvm: address should exist");
            }

            *pte = (*pte & ~PTE_A);//Clear referenced flag

            proc->numOfPages++;
        }
        else{//Init and shell:
            mappages(pgdir, (char *) a, PGSIZE, v2p(mem), PTE_W | PTE_U);
            proc->numOfPages++;
        }
    }
    cprintf("Here - proc: %d\n", proc->pid);

    return newsz;
}

void updateStatsOnPageEntrance(int pageNum) {
    //update the arrival time for all pages and NFU for new page:
    int i;
    for(i = 0; i < 30; i++){
        if(proc->fileData->arrivalTime[i] > 0) {
            proc->fileData->arrivalTime[i]++;
        }

    }
    proc->fileData->arrivalTime[pageNum] = 1;
    proc->fileData->nfuCounter[pageNum] = 0x8000;
}


// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
    pte_t *pte;
    uint a, pa;

    if (newsz >= oldsz)
        return oldsz;

    a = PGROUNDUP(newsz);
    for (; a  < oldsz; a += PGSIZE) {
        pte = walkpgdir(pgdir, (char*)a, 0);
        if (!pte)
            a += (NPTENTRIES - 1) * PGSIZE;
        else if ((*pte & PTE_P) != 0) {
            pa = PTE_ADDR(*pte);
            if (pa == 0)
                panic("kfree");
            char *v = p2v(pa);
            kfree(v);
            *pte = 0;
        }
    }

    if(proc->pgdir == pgdir){
        cprintf("deallocuvm reducing pages count...\n");
        proc->numOfPages = newsz/PGSIZE;
    }


    return newsz;
}

void updateAgingForNfu(){
    int i;
    for (i = 3; i < 30; i++) {
        int toShift = proc->fileData->nfuCounter[i];
        toShift = toShift >> 1;
        pte_t *pte;
        pte = walkpgdir(proc->pgdir, (void *) ((int)i * PGSIZE), 0);
        if ((*pte & PTE_A)) {//The page was accessed since last tick:
            toShift |= (0x8000);
            *pte &= (~PTE_A);
        }
        proc->fileData->nfuCounter[i] = toShift;
    }

}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
    uint i;
    if (pgdir == 0)
        panic("freevm: no pgdir");
    deallocuvm(pgdir, KERNBASE, 0);
    for (i = 0; i < NPDENTRIES; i++) {
        if (pgdir[i] & PTE_P) {
            char * v = p2v(PTE_ADDR(pgdir[i]));
            kfree(v);
        }
    }
    kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
    pte_t *pte;

    pte = walkpgdir(pgdir, uva, 0);
    if (pte == 0)
        panic("clearpteu");
    *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
    pde_t *d;
    pte_t *pte;
    uint pa, i, flags;
    char *mem;

    if ((d = setupkvm()) == 0)
        return 0;

    for (i = 0; i < sz; i += PGSIZE) {
        if ((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
            panic("copyuvm: pte should exist");
        if (!(*pte & PTE_P))
            panic("copyuvm: page not present");
        pa = PTE_ADDR(*pte);
        flags = PTE_FLAGS(*pte);
        if ((mem = kalloc()) == 0)
            goto bad;
        memmove(mem, (char*)p2v(pa), PGSIZE);
        if (mappages(d, (void*)i, PGSIZE, v2p(mem), flags) < 0)
            goto bad;
    }
    return d;

    bad:
    freevm(d);
    return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
    pte_t *pte;

    pte = walkpgdir(pgdir, uva, 0);
    if ((*pte & PTE_P) == 0)
        return 0;
    if ((*pte & PTE_U) == 0)
        return 0;
    return (char*)p2v(PTE_ADDR(*pte));
}

void deleteOnePage(){
    proc->numOfPages--;

    int lastPage = proc->numOfPages;
    proc->fileData->arrivalTime[lastPage] = 0;
    proc->fileData->nfuCounter[lastPage] = 0;

    pte_t* pte = walkpgdir(proc->pgdir, (void*)(lastPage*PGSIZE) , 0);
    uint pa = PTE_ADDR(*pte);
    kfree(p2v(pa));
    *pte = (*pte & ~PTE_PG);//Turn on in hard disk flag
    *pte = (*pte & ~PTE_P);//turn off present flag
}

void removePages(int n){
    int i;
    for(i = 0; i < n; i++){
        deleteOnePage();
    }
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
    char *buf, *pa0;
    uint n, va0;

    buf = (char*)p;
    while (len > 0) {
        va0 = (uint)PGROUNDDOWN(va);
        pa0 = uva2ka(pgdir, (char*)va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (va - va0);
        if (n > len)
            n = len;
        memmove(pa0 + (va - va0), buf, n);
        len -= n;
        buf += n;
        va = va0 + PGSIZE;
    }
    return 0;
}



int fetchPage(int pageNum){
    cprintf("Fetching page: %d\n", pageNum);
    //Load page from swapFile into Buff:
    char buffFromFile[PGSIZE];
    char buffFromMemory[PGSIZE];

    loadPageFromSwapFile(buffFromFile, pageNum);

    //Remove a page from memory to file:
    void* removedPageAddress = selectOnePageToRemove();
//    char* mem;
//    mem = kalloc();
//    memmove(removedPageAddress, buff, PGSIZE);
    memmove(buffFromMemory, removedPageAddress, PGSIZE);

    //Now both buffers have data
    savePageInSwapFile(buffFromMemory, pageNum);
    memmove(removedPageAddress, buffFromFile, PGSIZE);
    mappages(proc->pgdir, (char *) (pageNum*PGSIZE), PGSIZE, v2p(removedPageAddress), PTE_W | PTE_U);

    pte_t *pte = walkpgdir(proc->pgdir, (void*)(pageNum*PGSIZE), 0);
    updateStatsOnPageEntrance(pageNum);

    *pte |= PTE_A; //Turn present on
    *pte &= (~PTE_PG); // turn paged out off
    *pte |= PTE_A; //Turn present on

    return 1;
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.



