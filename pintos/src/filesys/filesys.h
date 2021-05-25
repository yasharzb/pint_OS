#ifndef FILESYS_FILESYS_H
#define FILESYS_FILESYS_H

#include <stdbool.h>
#include "filesys/directory.h"
#include "filesys/off_t.h"
#include "filesys/file-descriptor.h"


/* Sectors of system file inodes. */
#define FREE_MAP_SECTOR 0       /* Free map file inode sector. */
#define ROOT_DIR_SECTOR 1       /* Root directory file inode sector. */

/* Block device that contains the file system. */
struct block *fs_device;

void filesys_init (bool format);
void filesys_done (void);
bool filesys_create (const char *name, off_t initial_size, bool isDir);
struct file *filesys_open (const char *path); // TODO
bool filesys_remove (const char *path); // TODO

struct inode *get_name_and_dir_from_path(const char *path, char **file_name, struct dir **file_dir);
struct dir* get_path_initial_directory(const char *path);
int get_path_next_token(const char **ptr, char **token);

#endif /* filesys/filesys.h */
