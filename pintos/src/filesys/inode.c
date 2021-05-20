#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define INODE_DIRECT_BLOCKS_COUNT 16
#define POINTER_BLOCK_POINTERS_COUNT (BLOCK_SECTOR_SIZE / 4)
#define MAX_FILE_SIZE ((POINTER_BLOCK_POINTERS_COUNT) * (POINTER_BLOCK_POINTERS_COUNT) * (BLOCK_SECTOR_SIZE))

struct pointer_block_disk
  {
    block_sector_t blocks[POINTER_BLOCK_POINTERS_COUNT];
  };



/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */

    // int type;
    // block_sector_t direct_blocks[INODE_DIRECT_BLOCKS_COUNT];
    // block_sector_t indirect_block;
    // block_sector_t double_indirect_block;
    // uint32_t unused[124];
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */


    // struct lock *access_lock; // lock for read/write synchronization
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
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
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
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
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (free_map_allocate (sectors, &disk_inode->start))
        {
          block_write (fs_device, sector, disk_inode);
          if (sectors > 0)
            {
              static char zeros[BLOCK_SECTOR_SIZE];
              size_t i;

              for (i = 0; i < sectors; i++)
                block_write (fs_device, disk_inode->start + i, zeros);
            }
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

static inline size_t
sectors_to_indirect_sectors (size_t sectors)
{
  return DIV_ROUND_UP (sectors, POINTER_BLOCK_POINTERS_COUNT);
}


block_sector_t *
allocate_k_sectors(size_t k) 
{
  block_sector_t *sectors = NULL;
  sectors = calloc(k, sizeof (block_sector_t));
  if (sectors != NULL) {
    size_t sector = 0;
    for (; sector < k; sector++)
        if (!free_map_allocate(1, &sectors[sector]))
          break;
    if (sector < k) 
      {
        for (size_t sector_ind = 0; sector_ind < k; sector_ind++)
          free_map_release(sectors[sector_ind], 1);
        free(sectors);
        return NULL;
      }
    return sectors;
  }
  return NULL;
}


struct pointer_block_disk **
allocate_k_sectors_in_ram(size_t k)
{

  struct disk_indirect_blocks **disk_indirect_blocks = calloc(k, sizeof(struct pointer_block_disk*));
  if (disk_indirect_blocks == NULL)
    return NULL;
  size_t ind = 0;
  for (; ind < k; ind++) 
    {
      disk_indirect_blocks[ind] = calloc(1, BLOCK_SECTOR_SIZE);
      if (disk_indirect_blocks[ind] == NULL)
        break;
    }
  if (ind < k)
    {
      for (size_t i = 0; i < ind; i++)
        free(disk_indirect_blocks[i]);
      return NULL;
    }
  return disk_indirect_blocks;
}


bool extend_inode_disk_to_size(struct inode_disk *disk_inode, size_t size) 
{
  ASSERT (size < MAX_FILE_SIZE);

  size_t current_sectors = bytes_to_sectors (disk_inode->length);
  size_t sectors_needed = bytes_to_sectors (size);

  size_t current_indirect_blocks = sectors_to_indirect_sectors (current_sectors);
  size_t indirect_blocks_needed = sectors_to_indirect_sectors (sectors_needed);

  size_t new_sectors = sectors_needed - current_sectors;
  size_t new_indirect_blocks = indirect_blocks_needed - current_indirect_blocks;

  block_sector_t *sectors = NULL;
  block_sector_t *indirect_sectors = NULL;


  if (new_indirect_blocks > 0)
    {
      indirect_sectors = allocate_k_sectors (new_indirect_blocks);
      if (indirect_sectors == NULL)
        return false;
    }
  
  struct pointer_block_disk *disk_indirect_block_buffer = NULL;
  if (new_sectors > 0)
    {
      disk_indirect_block_buffer = calloc(1, BLOCK_SECTOR_SIZE);
      if (disk_indirect_block_buffer == NULL)
        return false;
      sectors = allocate_k_sectors (new_sectors);
      if (sectors == NULL)
        return false;
    }
  

  struct pointer_block_disk *disk_double_indirect_block = calloc (1, BLOCK_SECTOR_SIZE);
  if (disk_double_indirect_block == NULL)
    return false;

  block_read(fs_device, disk_inode->double_indirect_block, disk_double_indirect_block);
  
  size_t indirect_block_to_build = current_indirect_blocks;
  for (; indirect_block_to_build < indirect_blocks_needed; indirect_blocks_needed++)
    disk_double_indirect_block->blocks[indirect_block_to_build] = 
    indirect_sectors[indirect_block_to_build - current_indirect_blocks];
  
  size_t sector_to_build = current_sectors;

  off_t last_indirect_block = -1;
  while (sector_to_build < sectors_needed) 
    {
      size_t indirect_block_ind = sector_to_build / (POINTER_BLOCK_POINTERS_COUNT * POINTER_BLOCK_POINTERS_COUNT);

      size_t last_sector_to_build_in_indirect_block = ROUND_UP(sector_to_build, POINTER_BLOCK_POINTERS_COUNT);
      if (last_sector_to_build_in_indirect_block < sectors_needed)
        last_sector_to_build_in_indirect_block = sectors_needed;
      

      size_t tmp = sector_to_build % POINTER_BLOCK_POINTERS_COUNT;

      size_t last_indirect_block_ind = (sector_to_build) / POINTER_BLOCK_POINTERS_COUNT;

      block_read(fs_device, disk_double_indirect_block->blocks[indirect_block_ind], (void *) disk_indirect_block_buffer);

      for (size_t ind = sector_to_build, i = sector_to_build % POINTER_BLOCK_POINTERS_COUNT; ind < last_sector_to_build_in_indirect_block; ind++, i++) 
        {
          disk_indirect_block_buffer->blocks[i] = sectors[ind - sector_to_build];
        }
      
      block_write(fs_device, disk_double_indirect_block->blocks[indirect_block_ind], disk_indirect_block_buffer);
    }

  disk_inode->length = size;
  return true;
}


bool
inode_create_new (block_sector_t sector, off_t length)
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
      size_t sectors = bytes_to_sectors (length);
      // disk_inode->length = length;
      disk_inode->length = 0;
      disk_inode->magic = INODE_MAGIC;

      /*

      size_t remaining_sectors = sectors;
      bool allocation_failed = false;

      size_t diret_sectors = INODE_DIRECT_BLOCKS_COUNT;
      if (sectors < diret_sectors)
        diret_sectors = sectors;

      size_t sectors_allocated = 0;
      for (; sectors_allocated < sectors; sectors_allocated++)
        if (!free_map_allocate(1, &disk_inode[sectors_allocated])) 
          {
            allocation_failed = true;
            break;    
          }

      if (allocation_failed) {
        for (size_t i = 0; i < sectors; i++) 
          free_map_release(disk_inode->direct_blocks[i], 1);
        goto done;
      }

      remaining_sectors -= diret_sectors;

      if (remaining_sectors == 0)
        {
          success = true;
          goto done;
        }
      
      size_t indirect_sectors = POINTER_BLOCK_POINTERS_COUNT;
      if (remaining_sectors < indirect_sectors)
        indirect_sectors = remaining_sectors;
      */

      if (free_map_allocate (sectors, &disk_inode->double_indirect_block))
        {
          block_write (fs_device, sector, disk_inode);
          static char zeros[BLOCK_SECTOR_SIZE];
          block_write (fs_device, disk_inode->double_indirect_block, zeros);
          success = true;

          extend_inode_disk_to_size(disk_inode, length);
        }
      
      free (disk_inode);
    }

  
  return success;
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

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          free_map_release (inode->data.start,
                            bytes_to_sectors (inode->data.length));
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          block_read (fs_device, sector_idx, buffer + bytes_read);
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          block_read (fs_device, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

off_t
inode_read_at_new (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  struct pointer_block_disk *double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (double_indirect_block_buffer == NULL)
    return 0;

  struct pointer_block_disk *indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (indirect_block_buffer == NULL)
    return false;

  void *sector_buffer = NULL;

  block_read (fs_device, inode->data.double_indirect_block, double_indirect_block_buffer);
  size_t last_indirect_block_ind = -1;
  while (size > 0)
    {
      size_t sector_offset = offset % BLOCK_SECTOR_SIZE;
      size_t sector_ind = offset / BLOCK_SECTOR_SIZE;
      size_t indirect_block_ind = sector_ind / POINTER_BLOCK_POINTERS_COUNT;
      size_t sector_ind_in_indirect_block = sector_ind % POINTER_BLOCK_POINTERS_COUNT;
      size_t bytes_left_in_sector = BLOCK_SECTOR_SIZE - offset;
      size_t chunk_size = bytes_left_in_sector < size ? bytes_left_in_sector : size;

      if (last_indirect_block_ind != indirect_block_ind)
        {
          block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
          last_indirect_block_ind = indirect_block_ind;
        }

      if (sector_buffer == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          block_write (fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_read);
        }
      else
        {
          sector_buffer = malloc(BLOCK_SECTOR_SIZE);
          if (sector_buffer == NULL)
            break;
          block_sector_t sector = indirect_block_buffer->blocks[sector_ind_in_indirect_block];
          block_read (fs_device, sector, sector_buffer);
          memcpy (buffer + bytes_read, sector_buffer + sector_offset, chunk_size);
        }
      bytes_read += chunk_size;
      offset += chunk_size;
      size -= chunk_size;
    }
  
  free(double_indirect_block_buffer);
  free(indirect_block_buffer);
  if (sector_buffer != NULL)
    free(sector_buffer);
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          block_write (fs_device, sector_idx, buffer + bytes_written);
        }
      else
        {
          /* We need a bounce buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left)
            block_read (fs_device, sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

off_t
inode_write_at_new (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;
  
  if (offset > MAX_FILE_SIZE || size == 0)
    return 0;
  
  if (offset + size > MAX_FILE_SIZE)
    size = MAX_FILE_SIZE - offset;
  
  extend_inode_disk_to_size (&inode->data, offset + size);

  struct pointer_block_disk *double_indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (double_indirect_block_buffer == NULL)
    return 0;

  struct pointer_block_disk *indirect_block_buffer = malloc (BLOCK_SECTOR_SIZE);
  if (indirect_block_buffer == NULL)
    return 0;

  void *sector_buffer;

  block_read (fs_device, inode->data.double_indirect_block, double_indirect_block_buffer);
  size_t last_indirect_block_ind = -1;
  while (size > 0)
    {
      size_t sector_offset = offset % BLOCK_SECTOR_SIZE;
      size_t sector_ind = offset / BLOCK_SECTOR_SIZE;
      size_t indirect_block_ind = sector_ind / POINTER_BLOCK_POINTERS_COUNT;
      size_t sector_ind_in_indirect_block = sector_ind % POINTER_BLOCK_POINTERS_COUNT;
      size_t bytes_left_in_sector = BLOCK_SECTOR_SIZE - offset;
      size_t chunk_size = bytes_left_in_sector < size ? bytes_left_in_sector : size;

      if (last_indirect_block_ind != indirect_block_ind)
        {
          block_read (fs_device, double_indirect_block_buffer->blocks[indirect_block_ind], indirect_block_buffer);
          last_indirect_block_ind = indirect_block_ind;
        }

      if (sector_buffer == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          block_write (fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_written);
        }
      else
        {
          sector_buffer = malloc(BLOCK_SECTOR_SIZE);
          if (sector_buffer == NULL)
            return 0;
          block_sector_t sector = indirect_block_buffer->blocks[sector_ind_in_indirect_block];
          block_read (fs_device, sector, sector_buffer);
          memcpy (sector_buffer + sector_offset, buffer + bytes_written, chunk_size);
          block_write (fs_device, sector, sector_buffer);
        }
      bytes_written += chunk_size;
      offset += chunk_size;
      size -= chunk_size;
    }
  
  free(double_indirect_block_buffer);
  free(indirect_block_buffer);
  if (sector_buffer != NULL)
    free(sector_buffer);
  return bytes_written;
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
