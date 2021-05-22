#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create_name (const char *name, off_t initial_size, struct dir *dir)
{
  block_sector_t inode_sector = 0;
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, 0) //TODO isDir
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

bool
filesys_create (const char *path, off_t initial_size)
{
  //TODO
  return filesys_create_name(path, initial_size, get_path_initial_directory(path));
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open_name (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Opens the file with the given PATH. */
struct file *
filesys_open (const char *path)
{
  struct inode *inode = get_name_and_dir_from_path(path, NULL, NULL);
  return file_open (inode);
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove_name (const char *name)
{
  struct dir *dir = get_working_directory();
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);

  return success;
}

bool
filesys_remove (const char *path)
{
  struct inode *inode = get_name_and_dir_from_path(path, NULL, NULL);
  return filesys_remove_name(path);
}


/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16)) // TODO do we need to change this or not?
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}


/* Return file inode for path.
  Fill file name and closest dir from path if not null,
  Make sure to close inode and file_dir and free file_name!!!
 */
struct inode *
get_name_and_dir_from_path(const char *path, char **file_name, struct dir **file_dir)
{
  struct dir *dir = get_path_initial_directory(path);
  struct inode *mid_inode = NULL;
  struct inode *inode = NULL;
  
  if (dir == NULL) 
    goto fail;

  char *name = NULL;
  char *temp_name = NULL;

  int ret = get_path_next_token(&path, &name);
  if(ret <= 0)
    goto fail;

  while(dir != NULL) {
    /* find path token inode in dir */
    if(!dir_lookup (dir, name, &mid_inode))
      break;
    
    /* find next path token */
    if(temp_name)
      free(temp_name);
    temp_name = name; /* This is needed because if the path has ended we need the next to
                           last name to copy to file_name. last one is null. */
    ret = get_path_next_token(&path, &name);
    
    /* if it's last token then it's done */
    if(ret == 0) {
      inode = inode_reopen(mid_inode);
      
      if(file_name) {
        *file_name = malloc(NAME_MAX + 1);
        memcpy(*file_name, temp_name, strlen(temp_name));
      }

      if(file_dir)
        *file_dir = dir_reopen(dir);
      goto done;
    }

    if(ret < 0)
      goto fail;

    /* check if mid path inode is directory */
    if(!(mid_inode->data).isDir)
      break;
  
    dir_close(dir);   // last mid_inode will be closed here
    dir = dir_open(mid_inode);
  }

fail:
  if(file_dir)
    *file_dir = NULL;
  if(file_name)
    *file_name = NULL;

done:
  inode_close(mid_inode);
  dir_close (dir);
  if(name)
    free(name);
  if(temp_name)
    free(temp_name);

  return inode;
}



/* Get directory that the path need to start from. Don't forget to
  close the dir after using this function! */
struct dir*
get_path_initial_directory(const char *path) 
{
  if(*path == '/')
    return dir_open_root();
  
  return dir_reopen(get_working_directory());
}

/* Get next part of path that it's pointer is pointed by ptr! and return the part 
  in token. Return 0 at end of str and -1 if token length is too long */
int
get_path_next_token(const char **ptr, char **returned_token) 
{
  *returned_token = NULL;

  char *token = malloc(NAME_MAX + 1);
  if(!token) {
    return -1;
  }

  const char *path = *ptr;
  
  /* Skip slashes */
  while(*path == '/')
    path++;

  /* End of string */
  if(!(*path)) {
    free(token);
    return 0;
  }

  /* Copy from path to token */
  int len = 0;
  while(*path && (*token) != '/') 
  {
    if(len == NAME_MAX) {
      /* File name too long */
      free(token);
      return -1;
    }

    *(token+len) = *path;
    path++;
    len++;
  }
  
  /* Null terminate string */
  *(token+len) = 0;

  /* Change ptr to not include this part */
  *ptr = path;

  *returned_token = token;

  return 1;
}
