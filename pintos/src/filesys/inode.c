#include "filesys/inode.h"
#include "filesys/cache.h"
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}


block_sector_t *allocate_k_sectors (size_t k);
bool extend_inode_disk_to_size (block_sector_t inode_disk_sector, struct inode_disk *disk_inode, off_t size);
void free_block_sector_array(block_sector_t *arr, size_t k);

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
 block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
 ASSERT (inode != NULL);
 if (pos < inode->data.length)
   return inode->data.start + pos / BLOCK_SECTOR_SIZE;
 else
   return -1;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  init_cache();
}



/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init(&inode->access_lock);
  block_read (fs_device, inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}


/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}


/********************** NEW FUNCTIONS ***********************/

static inline size_t
sectors_to_indirect_blocks (size_t sectors)
{
  return DIV_ROUND_UP (sectors, POINTER_BLOCK_POINTERS_COUNT);
}

void 
free_block_sector_array(block_sector_t *arr, size_t k)
{
  for (size_t i = 0; i < k; i++)
    free_map_release(arr[i], 1);
}

block_sector_t *
allocate_k_sectors(size_t k) 
{
  block_sector_t *sectors = NULL;
  sectors = calloc(k, sizeof (block_sector_t));
  if (sectors != NULL) {
    size_t sectors_len = 0;
    for (; sectors_len < k; sectors_len++)
        if (!free_map_allocate(1, &sectors[sectors_len]))
          break;
    if (sectors_len < k) 
      {
        free_block_sector_array(sectors, sectors_len);
        free(sectors);
        return NULL;
      }
    return sectors;
  }
  return NULL;
}

bool 
extend_inode_disk_to_size (block_sector_t inode_disk_sector, struct inode_disk *disk_inode, off_t size) 
{
  ASSERT (size < MAX_FILE_SIZE);
  ASSERT (size >= 0);

  if (size < disk_inode->length)
    return true;

  off_t current_sectors = bytes_to_sectors (disk_inode->length);
  off_t sectors_needed = bytes_to_sectors (size);

  off_t current_indirect_blocks = sectors_to_indirect_blocks (current_sectors);
  off_t indirect_blocks_needed = sectors_to_indirect_blocks (sectors_needed);

  off_t new_sectors = sectors_needed - current_sectors;
  off_t new_indirect_blocks = indirect_blocks_needed - current_indirect_blocks;

  ASSERT (new_sectors >= 0);
  ASSERT (new_indirect_blocks >= 0);

  block_sector_t *sectors = NULL;
  block_sector_t *indirect_blocks = NULL;

  struct pointer_block_disk *indirect_block_buffer = NULL;
  struct pointer_block_disk *double_indirect_block_buffer = NULL;

  bool success = false;

  if (new_sectors == 0)
    {
      success = true;
      goto done;
    }

  if (new_indirect_blocks > 0)
    {
      indirect_blocks = allocate_k_sectors (new_indirect_blocks);
      if (indirect_blocks == NULL)
        goto done;
    }
  
  indirect_block_buffer = malloc(BLOCK_SECTOR_SIZE);
  if (indirect_block_buffer == NULL)
    goto done;
  sectors = allocate_k_sectors(new_sectors);
  if (sectors == NULL)
    goto done;

  double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (double_indirect_block_buffer == NULL)
    goto done;

  block_read(fs_device, disk_inode->double_indirect_block, double_indirect_block_buffer);
  
  off_t indirect_block_to_build = current_indirect_blocks;
  for (; indirect_block_to_build < indirect_blocks_needed; indirect_block_to_build++)
    double_indirect_block_buffer->blocks[indirect_block_to_build] = 
    indirect_blocks[indirect_block_to_build - current_indirect_blocks];
  block_write(fs_device, disk_inode->double_indirect_block, double_indirect_block_buffer);
  
  off_t last_indirect_block_ind = -1;
  for (off_t sector_to_build = current_sectors; sector_to_build < sectors_needed; sector_to_build++)
    {
      off_t indirect_block_ind = sector_to_build / POINTER_BLOCK_POINTERS_COUNT;

      if (last_indirect_block_ind != indirect_block_ind)
        {
          if (last_indirect_block_ind != -1)
            block_write (fs_device, double_indirect_block_buffer->blocks[last_indirect_block_ind], indirect_block_buffer);
          last_indirect_block_ind = indirect_block_ind;
          block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
        }
      
      off_t sector_ind_in_indirect_block = sector_to_build % POINTER_BLOCK_POINTERS_COUNT;
      indirect_block_buffer->blocks[sector_ind_in_indirect_block] = sectors[sector_to_build - current_sectors];
    }
  
  if (last_indirect_block_ind != -1)
    block_write(fs_device, double_indirect_block_buffer->blocks[last_indirect_block_ind], indirect_block_buffer);

  success = true;

done:

  if (sectors != NULL)
    {
      if (!success)
        free_block_sector_array(sectors, new_sectors);
      free (sectors);
    }
  if (indirect_blocks != NULL) 
    {
      if (!success)
        free_block_sector_array(indirect_blocks, new_indirect_blocks);
      free (indirect_blocks);
    }
  if (indirect_block_buffer != NULL)
    free (indirect_block_buffer);
  if (double_indirect_block_buffer != NULL)
    free (double_indirect_block_buffer);

  if (success) 
    {
      disk_inode->length = size;
      block_write(fs_device, inode_disk_sector, disk_inode);
    }
  return success;
}


bool
inode_create (block_sector_t sector, off_t length, int isDir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->length = 0;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->isDir = isDir;

      if (free_map_allocate (1, &disk_inode->double_indirect_block))
        {
          if (extend_inode_disk_to_size(sector, disk_inode, length))
            success = true;
          else
            free_map_release(disk_inode->double_indirect_block, 1);
        }
      
      free (disk_inode);
    }

  return success;
}


void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  lock_acquire(&(inode->access_lock));

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          off_t size = inode->data.length;
          off_t sectors = bytes_to_sectors (size);
          off_t indirect_blocks = sectors_to_indirect_blocks (sectors);

          struct pointer_block_disk *double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
          struct pointer_block_disk *indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);

          if (double_indirect_block_buffer == NULL || indirect_block_buffer == NULL) 
            {
              ASSERT (1 == 0); // shit!
            }
          
          block_read (fs_device, inode->data.double_indirect_block, double_indirect_block_buffer);
          
          off_t last_indirect_block_ind = -1;
          for (off_t i = 0; i < sectors; i++)
            {
              off_t indirect_block_ind = i / POINTER_BLOCK_POINTERS_COUNT;
              if (last_indirect_block_ind != indirect_block_ind)
                {
                  last_indirect_block_ind = indirect_block_ind;
                  block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
                }
              off_t sector_ind_in_indirect_block = i % POINTER_BLOCK_POINTERS_COUNT;
              free_map_release (indirect_block_buffer->blocks[sector_ind_in_indirect_block], 1);
            }
          free (indirect_block_buffer);
          
          for (off_t i = 0; i < indirect_blocks; i++)
            free_map_release (double_indirect_block_buffer->blocks[i], 1);
          free (double_indirect_block_buffer);
          
          free_map_release (inode->data.double_indirect_block, 1);
          free_map_release (inode->sector, 1);
        }

      lock_release(&(inode->access_lock));
      free (inode);
      return;
    }

  lock_release(&(inode->access_lock));
  return;
}


off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  struct pointer_block_disk *double_indirect_block_buffer = NULL; 
  struct pointer_block_disk *indirect_block_buffer = NULL;
  void *sector_buffer = NULL;

  // lock_acquire (&inode->access_lock);

  if (offset > inode->data.length)
    goto done;
  
  if (size + offset > inode->data.length)
    size = inode->data.length - offset;

  double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (double_indirect_block_buffer == NULL)
    goto done;

  indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (indirect_block_buffer == NULL)
    goto done;

  block_read (fs_device, inode->data.double_indirect_block, double_indirect_block_buffer);
  off_t last_indirect_block_ind = -1;
  while (size > 0)
    {
      off_t sector_offset = offset % BLOCK_SECTOR_SIZE;
      off_t sector_ind = offset / BLOCK_SECTOR_SIZE;
      off_t indirect_block_ind = sector_ind / POINTER_BLOCK_POINTERS_COUNT;
      off_t sector_ind_in_indirect_block = sector_ind % POINTER_BLOCK_POINTERS_COUNT;
      off_t bytes_left_in_sector = BLOCK_SECTOR_SIZE - sector_offset;
      off_t chunk_size = bytes_left_in_sector < size ? bytes_left_in_sector : size;

      if (last_indirect_block_ind != indirect_block_ind)
        {
          block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
          last_indirect_block_ind = indirect_block_ind;
        }

      block_sector_t sector = indirect_block_buffer->blocks[sector_ind_in_indirect_block];
      cache *ca = get_cache(sector);
      if (ca != NULL)
      {
        memcpy(buffer + bytes_read, ca->data + sector_offset, chunk_size);
      }
      else {
        if (sector_offset == 0 && chunk_size == BLOCK_SECTOR_SIZE)
          {
            block_read (fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_read);
            set_cache(sector, buffer + bytes_read);
          }
        else
          {
            if (sector_buffer == NULL)
              sector_buffer = malloc(BLOCK_SECTOR_SIZE);
            if (sector_buffer == NULL)
              break;
            block_read (fs_device, sector, sector_buffer);
            set_cache(sector, sector_buffer);
            memcpy (buffer + bytes_read, sector_buffer + sector_offset, chunk_size);
          }
      }
      bytes_read += chunk_size;
      offset += chunk_size;
      size -= chunk_size;
    }
  
  // lock_release (&inode->access_lock);

done:
  if (double_indirect_block_buffer != NULL)
    free(double_indirect_block_buffer);
  if (indirect_block_buffer != NULL)
    free(indirect_block_buffer);
  if (sector_buffer != NULL)
    free(sector_buffer);
  return bytes_read;
}


off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  struct pointer_block_disk *double_indirect_block_buffer = NULL;
  struct pointer_block_disk *indirect_block_buffer = NULL;
  void *sector_buffer = NULL;

  // lock_acquire (&inode->access_lock);

  if (inode->deny_write_cnt)
    goto done;
  
  if (size == 0)
    goto done;
  
  
  if (size + offset > MAX_FILE_SIZE)
    size = MAX_FILE_SIZE - offset;
  
  if (!extend_inode_disk_to_size (inode->sector, &inode->data, offset + size))
    {
      if (offset < inode->data.length)
        size = inode->data.length - offset;
      else
        goto done;
    }

  double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (double_indirect_block_buffer == NULL)
    goto done;

  indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (indirect_block_buffer == NULL)
    goto done;

  block_read (fs_device, inode->data.double_indirect_block, double_indirect_block_buffer);
  off_t last_indirect_block_ind = -1;
  while (size > 0)
    {
      off_t sector_offset = offset % BLOCK_SECTOR_SIZE;
      off_t sector_ind = offset / BLOCK_SECTOR_SIZE;
      off_t indirect_block_ind = sector_ind / POINTER_BLOCK_POINTERS_COUNT;
      off_t sector_ind_in_indirect_block = sector_ind % POINTER_BLOCK_POINTERS_COUNT;
      off_t bytes_left_in_sector = BLOCK_SECTOR_SIZE - sector_offset;
      off_t chunk_size = bytes_left_in_sector < size ? bytes_left_in_sector : size;

      if (last_indirect_block_ind != indirect_block_ind)
        {
          block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
          last_indirect_block_ind = indirect_block_ind;
        }
      
      block_sector_t sector = indirect_block_buffer->blocks[sector_ind_in_indirect_block];
      cache *ca = get_cache(sector);

      if (ca != NULL)
        {
          update_cache(ca, buffer + bytes_written, sector_offset, chunk_size);
        }
      else {
        if (sector_offset == 0 && chunk_size == BLOCK_SECTOR_SIZE)
          {
            block_write (fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_written);
            set_cache(sector, buffer + bytes_written);
          }
        else
          {
            if (sector_buffer == NULL)
              sector_buffer = malloc(BLOCK_SECTOR_SIZE);
            if (sector_buffer == NULL)
              break;
            block_read (fs_device, sector, sector_buffer);
            memcpy (sector_buffer + sector_offset, buffer + bytes_written, chunk_size);
            block_write (fs_device, sector, sector_buffer);
            set_cache(sector, sector_buffer);
          }
      }
      bytes_written += chunk_size;
      offset += chunk_size;
      size -= chunk_size;
    }

  // lock_release (&inode->access_lock);
  
done:
  if (double_indirect_block_buffer != NULL)  
    free(double_indirect_block_buffer);
  if (indirect_block_buffer != NULL)
    free(indirect_block_buffer);
  if (sector_buffer != NULL)
    free(sector_buffer);
  return bytes_written;
}

