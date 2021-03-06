#include "filesys/cache.h"
#include "filesys/filesys.h"
#include <string.h>
#include "threads/malloc.h"

static struct list cached_blocks;
static int cache_size;
static struct list_elem *cache_pointer;

static bool index_cmp(const struct list_elem *a,
                      const struct list_elem *b, void *aux UNUSED);

void init_cache(void)
{
        list_init(&cached_blocks);
        cache_size = 0;
        cache_pointer = NULL;
}
cache *get_cache(block_sector_t index)
{
        struct list_elem *c_el = NULL;
        cache *ca = NULL;
        for (c_el = list_begin(&cached_blocks); c_el != list_end(&cached_blocks); c_el = list_next(c_el))
        {
                ca = list_entry(c_el, cache, elem);
                if (ca->index == index)
                        return ca;
                if (ca->index > index)
                        break;
        }
        return NULL;
}
void set_cache(block_sector_t index, const uint8_t *buf)
{
        cache *ca = malloc(sizeof(cache));
        if (ca == NULL)
                return;
        uint8_t *data = malloc(sizeof(uint8_t) * BLOCK_SECTOR_SIZE);
        if (data == NULL)
        {
                free(ca);
                return;
        }

        memcpy(data, buf, BLOCK_SECTOR_SIZE);
        ca->index = index;
        ca->data = data;
        ca->is_dirty = 0;
        ca->is_used = 1;

        // if (cache_size == CACHE_BLOCK_SIZE)
        //         apply_clock_algorithm();
        // else
        //         cache_size++;
        // list_insert_ordered(&cached_blocks, &ca->elem, index_cmp, NULL);
        list_insert_ordered(&cached_blocks, &ca->elem, index_cmp, NULL);
        cache_size++;
        if (cache_size > CACHE_BLOCK_SIZE)
                apply_clock_algorithm();

}
void update_cache(cache *ca, const uint8_t *buf, int offset, int size)
{
        memcpy(ca->data + offset, buf, size);
        ca->is_dirty = 1;
        ca->is_used = 1;
}
void remove_cache(cache *ca)
{
        list_remove(&ca->elem);
        free(ca->data);
        free(ca);
        cache_size--;
}
bool index_cmp(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
        cache *ca_a = list_entry(a, cache, elem);
        cache *ca_b = list_entry(b, cache, elem);
        return ca_a->index < ca_b->index;
}
void apply_clock_algorithm()
{
        cache *ca = NULL;

        if (cache_pointer == NULL || cache_pointer == list_end(&cached_blocks))
                cache_pointer = list_begin(&cached_blocks);

        while (1)
        {
                ca = list_entry(cache_pointer, cache, elem);
                if (ca->is_dirty)
                {
                        write_back(ca, true);
                }
                if (!ca->is_used)
                {
                        cache_pointer = list_next(cache_pointer);
                        remove_cache(ca);
                        return;
                }
                ca->is_used = 0;
                cache_pointer = list_next(cache_pointer);
                if (cache_pointer == list_end(&cached_blocks))
                        cache_pointer = list_begin(&cached_blocks);
        }
}

void write_back(cache *ca, bool mode)
{
        block_write(fs_device, ca->index, ca->data, mode);
        ca->is_dirty = 0;
}

void write_back_all_cache()
{
        struct list_elem *e;
        for (e = list_begin(&cached_blocks); e != list_end(&cached_blocks); e = list_next(e))
        {
                cache *ca = list_entry(e, cache, elem);
                if (ca->is_dirty)
                {
                        write_back(ca, false);
                }
        }
}
