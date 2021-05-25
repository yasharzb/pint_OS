#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include <list.h>
#include "devices/block.h"

#define CACHE_BLOCK_SIZE 64

typedef struct cache
{
    block_sector_t index;
    int is_dirty;
    int is_used;
    uint8_t *data;
    struct list_elem elem;
} cache;

void init_cache(void);
cache *get_cache(block_sector_t index);
void set_cache(block_sector_t index, const uint8_t *buf);
void update_cache(cache *ca, const uint8_t *buf, int offset, int size);
void remove_cache(cache *ca);

void write_back_all_cache(void);

#endif
