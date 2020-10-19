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
    if ((uint64_t)va > (1 << (12 + 9 + 9 + 9 - 1))) {
        panic("walk");
    }

    for (int level = 0; level < 3; level++) {
        uint64_t* pte = &pgdir[PTX(level, (uint64_t)va)];
        if (*pte & PTE_P) {
            // hit
            pgdir = (uint64_t*)PTE_ADDR(*pte);
        }
        else {
            if (!alloc || ((pgdir = (uint64_t*)kalloc()) == NULL)) {
                return NULL;
            }
            memset(pgdir, 0, PGSIZE);
            *pte = (((uint64_t)pgdir >> 12) << 10);
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
}