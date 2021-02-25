#ifndef BCACHE_H
#define BCACHE_H

#include "buf.h"

#include "spinlock.h"
#define CACHE_SZ 0x1000

struct {
    int valid;
    int blocknum;
    uint8_t data[BSIZE];
    struct spinlock lock;
} bcache_lo[CACHE_SZ];

static void bcache_init() {
    for (int i = 0; i < CACHE_SZ; i++) {
        bcache_lo[i].valid = 0;
        initlock(&bcache_lo[i].lock, "bcache");
    }
}

#endif