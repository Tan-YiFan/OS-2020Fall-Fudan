#include <stdint.h>
#include "types.h"
#include "mmu.h"
#include "string.h"
#include "memlayout.h"
#include "console.h"

#include "vm.h"
#include "kalloc.h"

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
 * Use permission bits perm|PTE_P|PTE_TABLE|PTE_AF for the entries.
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
        *pte = V2P(pa) | perm | PTE_P | PTE_TABLE | PTE_AF;
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
    switch (level) {
    case 3:
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_P) {
                kfree((char*)PTE_ADDR(pgdir[i]));
            }
        }
        break;
    case 0:
    case 1:
    case 2:
        for (int i = 0; i < 512; i++) {
            if (pgdir[i] & PTE_P) {
                vm_free((uint64_t*)PTE_ADDR(pgdir[i]), level + 1);
            }
        }
        break;
    default:
        panic("vm_free: unexpected level: %d\n", level);
        break;
    }
    return;
}