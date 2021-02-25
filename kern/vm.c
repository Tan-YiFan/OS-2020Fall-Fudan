#include <stdint.h>
#include "types.h"
#include "mmu.h"
#include "string.h"
#include "memlayout.h"
#include "console.h"

#include "vm.h"
#include "kalloc.h"
#include "proc.h"

#include "file.h"

extern uint64_t* kpgdir;

/*
 * Given 'pgdir', a pointer to a page directory, pgdir_walk returns
 * a pointer to the page table entry (PTE) for virtual address 'va'.
 * This requires walking the four-level page table structure.
 *
 * The relevant page table page might not exist yet.
 * If this is true, and alloc == false, then pgdir_walk returns NULL.
 * Otherwise, pgdir_walk allocates a new page table page with kalloc.
 *   - If the allocation fails, pgdir_walk returns NULL.
 *   - Otherwise, the new page is cleared, and pgdir_walk returns
 *     a pointer into the new page table page.
 */

static uint64_t*
pgdir_walk(uint64_t* pgdir, const void* va, int64_t alloc)
{
    /* TODO: Your code here. */
    if ((uint64_t)va >= (1l << (12 + 9 + 9 + 9 + 9 - 1))) {
        panic("pgdir_walk");
    }

    for (int level = 0; level < 3; level++) {
        uint64_t* pte = &pgdir[PTX(level, (uint64_t)va)];
        if (*pte & PTE_P) {
            // hit
            assert(*pte & PTE_TABLE); // should be table instead of block
            pgdir = (uint64_t*)P2V(PTE_ADDR(*pte));
        }
        else {
            if (!alloc || ((pgdir = (uint64_t*)kalloc()) == NULL)) {
                return NULL;
            }
            memset(pgdir, 0, PGSIZE);
            *pte = V2P(pgdir) | PTE_P | PTE_PAGE;
        }
    }

    return &pgdir[PTX(3, va)];
}

/*
 * Create PTEs for virtual addresses starting at va that refer to
 * physical addresses starting at pa. va and size might **NOT**
 * be page-aligned.
 * Use permission bits perm|PTE_P|PTE_TABLE|(MT_NORMAL << 2)|PTE_AF|PTE_SH for the entries.
 *
 * Hint: call pgdir_walk to get the corresponding page table entry
 */

static int
map_region(uint64_t* pgdir, void* va, uint64_t size, uint64_t pa, int64_t perm)
{
    /* TODO: Your code here. */
    uint64_t a, last;
    uint64_t* pte;

    a = PTE_ADDR((uint64_t)va);
    last = PTE_ADDR((uint64_t)va + (uint64_t)size - 1ul);
    while (1) {
        if ((pte = pgdir_walk(pgdir, (const void*)a, 1)) == NULL) {
            return -1;
        }
        if (*pte & PTE_P) {
            panic("remap");
        }
        *pte = (pa) | perm | PTE_P | PTE_TABLE | (MT_NORMAL << 2) | PTE_AF | PTE_SH;
        if (a == last) {
            break;
        }
        a += PGSIZE;
        pa += PGSIZE;
    }
    return 0;
}

/*
 * Free a page table.
 *
 * Hint: You need to free all existing PTEs for this pgdir.
 */

void
vm_free(uint64_t* pgdir, int level)
{
    /* TODO: Your code here. */
    assert(((uint64_t)pgdir & 0xffful) == 0ul);
    switch (level) {
    case 3:
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_P) {
                kfree((char*)P2V(PTE_ADDR(pgdir[i])));
            }
        }
        break;
    case 0:
    case 1:
    case 2:
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_P) {
                vm_free((uint64_t*)(P2V(PTE_ADDR(pgdir[i]))), level + 1);
            }
        }
        kfree((char*)pgdir);
        break;
    default:
        panic("vm_free: unexpected level: %d\n", level);
        break;
    }
    return;
}

/* Get a new page table */
uint64_t*
pgdir_init()
{
    /* TODO: Your code here. */
    uint64_t* ret = (uint64_t*)kalloc();
    if (ret == NULL) {
        panic("pgdir_init: unable to allocate a page table");
    }
    memset(ret, 0, PGSIZE);
    return ret;
}

/*
 * Load binary code into address 0 of pgdir.
 * sz must be less than a page.
 * The page table entry should be set with
 * additional PTE_USER|PTE_RW|PTE_PAGE permission
 */
void
uvm_init(uint64_t* pgdir, char* binary, int sz)
{
    /* TODO: Your code here. */
    if (sz >= PGSIZE) {
        panic("uvm_init: init process too big");
    }
    char* r = kalloc();
    if (r == NULL) {
        panic("uvm_init: cannot alloc a page");
    }
    memset(r, 0, PGSIZE);
    map_region(pgdir, (void*)0, PGSIZE, V2P(r), PTE_USER | PTE_RW | PTE_PAGE);
    memmove(r, (void*)binary, sz);
}

/*
 * switch to the process's own page table for execution of it
 */
void
uvm_switch(struct proc* p)
{
    /* TODO: Your code here. */
    if (p->pgdir == NULL) {
        panic("uvm_switch: pgdir is null pointer");
    }
    lttbr0(V2P(p->pgdir));
}

uint64_t* 
copyuvm(uint64_t* pgdir, uint32_t sz)
{
    uint64_t* new_pgdir = pgdir_init();
    if (new_pgdir == 0) {
        return 0;
    } 
    
    for (uint64_t i = 0; i < sz; i += PGSIZE) {
        uint64_t* pte = pgdir_walk(pgdir, (void*)i, 0);
        if (pte == 0) {
            panic("copyuvm");
        } 
        if (((*pte) & (PTE_PAGE | PTE_P)) == 0) {
            panic("copyuvm");
        } 
        uint64_t pa = PTE_ADDR(*pte);
        int64_t perm = *pte & (PTE_USER | PTE_RO);
        uint64_t* page = (uint64_t*)kalloc();
        if (page == 0) {
            kfree((char*)new_pgdir);
            return 0;
        } 
        memcpy(page, P2V(pa), PGSIZE);
        if (map_region(new_pgdir, (void*) i, PGSIZE, V2P(page), perm) < 0) {
            kfree((char*)new_pgdir);
            return 0;
        } 
    }
    
    return new_pgdir;
}

int
allocuvm(uint64_t* pgdir, uint32_t oldsz, uint32_t newsz)
{
    if (newsz >= UADDR_SZ) {
        return 0;
    }

    if (newsz < oldsz) {
        return oldsz;
    }

    for (uint64_t a = ROUNDUP(oldsz, PGSIZE); a < newsz; a += PGSIZE) {
        char* mem = kalloc();
        if (mem == 0) {
            cprintf("allocuvm out of memory\n");
            deallocuvm(pgdir, newsz, oldsz);
            return 0;
        }
        memset(mem, 0, PGSIZE);
        map_region(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_USER);
    } 
    return newsz;
}

int deallocuvm(uint64_t* pgdir, uint32_t oldsz, uint32_t newsz)
{
    if (newsz >= oldsz) {
        return oldsz;
    }
    for (uint64_t a = ROUNDUP(newsz, PGSIZE); a < oldsz; a += PGSIZE) {
        uint64_t* pte = pgdir_walk(pgdir, (char*)a, 0);
        if (pte == 0) {
            a = ROUNDUP(a, BKSIZE);
        } else if ((*pte & (PTE_P | PTE_PAGE)) != 0) {
            uint64_t pa = PTE_ADDR(pte);
            if (pa == 0) {
                panic("deallocuvm\n");
            }
            kfree(P2V(pa));
            *pte = 0;
        } 
        
    }
    return newsz;
}

int loaduvm(uint64_t* pgdir, char* addr, struct inode* ip, uint32_t offset, uint32_t sz)
{
    if ((uint64_t)(addr - offset) % PGSIZE) {
        panic("loaduvm: addr 0x%p not page aligned\n", addr);
    }
    // first page
    do {
        uint64_t va = ROUNDDOWN((uint64_t)addr, PGSIZE);
        uint64_t* pte = pgdir_walk(pgdir, va, 0);
        if (pte == 0) {
            panic("loaduvm: addr 0x%p should exist\n", va);
        }
        uint64_t pa = PTE_ADDR(*pte);
        uint64_t start = (uint64_t)addr % PGSIZE;
        uint32_t n = MIN(sz, PGSIZE - start);
        if (readi(ip, P2V(pa + start), offset, n) != n) {
            return -1;
        }
        offset += n;
        addr += n;
        sz -= n;
    } while (0);
    for (uint32_t i = 0; i < sz; i += PGSIZE) {
        uint64_t* pte = pgdir_walk(pgdir, addr + i, 0);
        if (pte == 0) {
            panic("loaduvm: addr 0x%p should exist\n", addr + i);
        }
        uint64_t pa = PTE_ADDR(*pte);
        uint32_t n = MIN(sz - i, PGSIZE);
        if (readi(ip, P2V(pa), offset + i, n) != n) {
            return -1;
        } 
    }
    return 0;
}

void clearpteu(uint64_t* pgdir, char* va)
{
    uint64_t* pte = pgdir_walk(pgdir, va, 0);
    if (pte == 0) {
        panic("clearpteu: va 0x%p not found\n", va);
    }
    *pte = (*pte & ~(PTE_USER | PTE_RO)) | PTE_RW;
}
//PAGEBREAK!
// Map user virtual address to kernel address.
char* uva2ka (uint64_t *pgdir, char *uva)
{
    uint64_t *pte;

    pte = pgdir_walk(pgdir, uva, 0);

    // make sure it exists
    if ((*pte & (PTE_PAGE | PTE_P)) == 0) {
        return 0;
    }

    // make sure it is a user page
    if (((*pte & PTE_USER) == 0) || ((*pte & PTE_RO) != 0)) {
        return 0;
    }
    return (char*) P2V(PTE_ADDR(*pte));
}
int copyout(uint64_t* pgdir, uint32_t va, void* p, uint32_t len)
{
    char *buf, *pa0;
    uint64_t n, va0;

    buf = (char*) p;

    while (len > 0) {
        va0 = ROUNDDOWN(va, PGSIZE);
        pa0 = uva2ka(pgdir, (char*) va0);

        if (pa0 == 0) {
            return -1;
        }

        n = PGSIZE - (va - va0);

        if (n > len) {
            n = len;
        }

        memmove(pa0 + (va - va0), buf, n);

        len -= n;
        buf += n;
        va = va0 + PGSIZE;
    }

    return 0;
}