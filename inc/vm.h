#ifndef KERN_VM_H
#define KERN_VM_H

#include <stdint.h>
#include "proc.h"

void vm_free(uint64_t *, int);
void uvm_switch(struct proc *);
void uvm_init(uint64_t *, char *, int);

uint64_t *pgdir_init();

uint64_t* copyuvm(uint64_t* pgdir, uint32_t sz);
int allocuvm(uint64_t* pgdir, uint32_t oldsz, uint32_t newsz);
int deallocuvm(uint64_t* pgdir, uint32_t oldsz, uint32_t newsz);
int loaduvm(uint64_t* pgdir, char* addr, struct inode* ip, uint32_t offset, uint32_t sz);
void clearpteu(uint64_t* pgdir, char* va);
int copyout(uint64_t* pgdir, uint32_t va, void* p, uint32_t len);
#endif