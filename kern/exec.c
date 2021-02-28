#include <elf.h>

#include "trap.h"

#include "file.h"
#include "log.h"
#include "string.h"

#include "console.h"
#include "vm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"

int
execve(const char *path, char *const argv[], char *const envp[])
{
    cdebugf("exec start\n");
    struct inode* ip = namei((char*)path);
    if (ip == 0) {
        return -1;
    } 
    ilock(ip);

    // check elf header
    Elf64_Ehdr elf;
    uint64_t* pgdir = 0;
    if (readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf)) {
        goto bad;
    }
    if (strncmp((const char*)elf.e_ident, ELFMAG, 4)) {
        cprintf("exec: magic not match, path %s\n", path);
        goto bad;
    }

    if ((pgdir = pgdir_init()) == 0) {
        goto bad;
    } 
    /* TODO: Load program into memory. */
    uint64_t sz = 0;
    Elf64_Phdr ph;
    for (int i = 0, off = elf.e_phoff; i < elf.e_phnum; i++, off += sizeof(ph)) {
        if (readi(ip, (char*) &ph, off, sizeof(ph)) != sizeof(ph)) {
            goto bad;
        }
        // cdebugf("%x %llx %llx\n", ph.p_type, ph.p_offset, ph.p_vaddr);
        if (ph.p_type != PT_LOAD) {
            // goto bad;
            continue;
        }
        if (ph.p_memsz < ph.p_filesz) {
            goto bad;
        }
        if ((sz = allocuvm(pgdir, sz, ph.p_vaddr + ph.p_memsz)) == 0) {
            goto bad;
        }
        if (loaduvm(pgdir, (char*)ph.p_vaddr, ip, ph.p_offset, ph.p_filesz) < 0) {
            goto bad;
        } 
    }
    iunlockput(ip);
    ip = 0;
    /* TODO: Allocate user stack. */
    sz = ROUNDUP(sz, PGSIZE);
    sz = allocuvm(pgdir, sz, sz + (PGSIZE << 1));
    if (!sz) {
        goto bad;
    }
    clearpteu(pgdir, (char*)(sz - (PGSIZE << 1)));
    uint64_t sp = sz;
    /* TODO: Push argument strings.
     *
     * The initial stack is like
     *
     *   +-------------+
     *   | auxv[o] = 0 | 
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   auxv[0]   |
     *   +-------------+
     *   | envp[m] = 0 |
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   envp[0]   |
     *   +-------------+
     *   | argv[n] = 0 |  n == argc
     *   +-------------+
     *   |    ....     |
     *   +-------------+
     *   |   argv[0]   |
     *   +-------------+
     *   |    argc     |
     *   +-------------+  <== sp
     *
     * where argv[i], envp[j] are 8-byte pointers and auxv[k] are
     * called auxiliary vectors, which are used to transfer certain
     * kernel level information to the user processes.
     *
     * ## Example 
     *
     * ```
     * sp -= 8; *(size_t *)sp = AT_NULL;
     * sp -= 8; *(size_t *)sp = PGSIZE;
     * sp -= 8; *(size_t *)sp = AT_PAGESZ;
     *
     * sp -= 8; *(size_t *)sp = 0;
     *
     * // envp here. Ignore it if your don't want to implement envp.
     *
     * sp -= 8; *(size_t *)sp = 0;
     *
     * // argv here.
     *
     * sp -= 8; *(size_t *)sp = argc;
     *
     * // Stack pointer must be aligned to 16B!
     *
     * thisproc()->tf->sp = sp;
     * ```
     *
     */

    
    int argc = 0;
    uint64_t ustack[3 + 32 + 1];

    for (; argv[argc]; argc++) {
        if (argc > 32) {
            goto bad;
        }
        sp -= strlen(argv[argc]) + 1;
        sp = ROUNDDOWN(sp, 16);
        if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0) {
            goto bad;
        } 

        ustack[argc] = sp;
    }
    ustack[argc] = 0;
    thisproc()->tf->r0 = argc;
    // thisproc()->tf->r1 = sp = ROUNDDOWN(sp - (argc + 1) * 8, 16);
    // auxv
    if ((argc & 1) == 0) {
        sp -= 8;
    } 
    uint64_t auxv[] = { 0, AT_PAGESZ, PGSIZE, AT_NULL };
    sp -= sizeof(auxv);
    // sp = ROUNDDOWN(sp, 16);
    if (copyout(pgdir, sp, auxv, sizeof(auxv)) < 0) {
        goto bad;
    } 
    // skip envp
    sp -= 8;
    uint64_t temp = 0;
    if (copyout(pgdir, sp, &temp, 8) < 0) {
        goto bad;
    } 
/*     if (argc & 1) {
        argc++;
        ustack[argc] = 0;
    } */
    thisproc()->tf->r1 = sp = sp - (argc + 1) * 8;
    if (copyout(pgdir, sp, ustack, (argc + 1) * 8) < 0) {
        goto bad;
    }
    sp -= 8;
    if (copyout(pgdir, sp, &thisproc()->tf->r0, 8) < 0) {
        goto bad;
    }
    uint64_t* oldpgdir = thisproc()->pgdir;
    thisproc()->pgdir = pgdir;
    thisproc()->sz = sz;
    thisproc()->tf->sp_el0 = sp;
    thisproc()->tf->elr_el1 = elf.e_entry;
    // assert((sp & 0xf) == 0);
    uvm_switch(thisproc());
    vm_free(oldpgdir, 1);
    cdebugf("exec return\n");
    return thisproc()->tf->r0;
bad:
    if (pgdir) {
        vm_free(pgdir, 1);
    }
    if (ip) {
        iunlockput(ip);
    }
    return -1;
}

