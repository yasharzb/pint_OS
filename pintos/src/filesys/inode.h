#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "filesys/file.h"
#include "threads/synch.h"

struct bitmap;

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

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
    block_sector_t start;                 /* First data sector. */
    off_t length;                         /* File size in bytes. */
    unsigned magic;                       /* Magic number. */
    uint32_t isDir;                       /* File is directory or not */
    block_sector_t double_indirect_block; /* Sector number of double pointer sector */
    uint32_t unused[123];                 /* Not used. */
  };

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct lock access_lock;            /* lock for read/write synchronization */
    struct inode_disk data;             /* Inode content. */
  };


void inode_init (void);
bool inode_create (block_sector_t, off_t, int);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

#endif /* filesys/inode.h */
