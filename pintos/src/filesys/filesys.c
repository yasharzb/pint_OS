#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/directory.h"



/* Partition that contains the file system. */
struct block *fs_device;

//struct inode;

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
filesys_create_name (char *name, off_t initial_size, struct dir *dir, bool isDir)
{
  block_sector_t inode_sector = 0;
  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, isDir)
                  && dir_add (dir, name, inode_sector));
  if (!success && inode_sector != 0)
    free_map_release (inode_sector, 1);
  
  dir_close (dir);
  free(name);

  return success;
}

bool
filesys_create (const char *path, off_t initial_size, bool isDir)
{
  struct dir* dir;
  char *name;

  struct inode *inode = get_name_and_dir_from_path(path, &name, &dir, 0);
  /* if inode of current path is found then it's failed */
  if(inode) 
    return false;
  /* if dir is not found then it's failed */
  if(!dir)
    return false;

  return filesys_create_name(name, initial_size, dir, isDir);
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open_name (const char *name)
{
  struct dir *dir = dir_open_root (); // TODO should we always look for it from root?
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
  struct inode *inode = get_name_and_dir_from_path(path, NULL, NULL, 0);
  return file_open (inode);
}


/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove_name (char *name, struct dir* dir)
{
  bool success = dir != NULL && dir_remove (dir, name);
  dir_close (dir);
  free(name);
  return success;
}


/* Deletes the file from given PATH */
bool
filesys_remove (const char *path)
{
  struct dir* dir;
  char *name;
  struct inode *inode = get_name_and_dir_from_path(path, &name, &dir, 0);
  inode_close(inode);
  return filesys_remove_name(name, dir);
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


/* Return file inode for path if exist. 
  Fill file name and closest dir from path if not null,
  Make sure to close inode and file_dir and free file_name!!!
  
  assume you have a path like `a/b/c`.
  If `a/b/c` exists then inode, file_name and file_dir are set accordingly.
  If `a/b` exists but `a/b/c` doesn't exist, inode will be null but file_name 
  file_dir will be set.
  O.W. everything will be null.
 */
struct inode *
get_name_and_dir_from_path(const char *path, char **file_name, struct dir **file_dir, int situation)
{
  struct dir *dir = get_path_initial_directory(path);
  struct inode *mid_inode = NULL;
  struct inode *inode = NULL;
  
  char *token = NULL;
  char *next_token = NULL;
  
  if (dir == NULL) 
    goto fail;

  int ret = get_path_next_token(&path, &token);
  if(ret <= 0)
    goto fail;

  while(dir != NULL) {
    /* find path token inode in dir */
    bool found = dir_lookup (dir, token, &mid_inode);
    
    /* find next path token. it's needed to know if the current token is last token*/
    ret = get_path_next_token(&path, &next_token);
    if(ret < 0)
      goto fail;
    
    /* if it's last token then it's done */
    if(ret == 0) {
      /* if last token is found then we will return it, o.w. inode remains null */
      if(found)
        inode = inode_reopen(mid_inode);
      

      /** Well if mkdir is called with file_name and file_dir both set to NULL,
       * this part is not needed. But I'm not quite sure yet.

      if(file_name) {
        *file_name = malloc(NAME_MAX + 1);
        memcpy(*file_name, token, strlen(token) + 1);
      }

      if(file_dir)
        *file_dir = dir_reopen(dir);

      */

      goto done;
    }

    /* if token it's not last token and it's not found in dir then search is failed */
    if(!found){
      if(situation == 0)
        break;
      else if(situation == 1) {
        /* If we are here, the path from now on doesn't exist. We have to create it. */
        if(!filesys_create_name(token, 16 * sizeof(struct dir_entry), dir, 1))
          goto fail;
        /* new directory should be added now; we look for it in dir */
        bool now_found = dir_lookup (dir, token, &mid_inode);
        if(ret == 0) {

          if(now_found)
            inode = inode_reopen(mid_inode);

          if(file_name) {
            *file_name = malloc(NAME_MAX + 1);
            memcpy(*file_name, token, strlen(token) + 1);
          }

          if(file_dir)
            *file_dir = dir_reopen(dir);
          
          goto done;
        }

        /* if we still can't find that directory something bad must've happened. :( */

        if(!now_found)
          break;
      }
    }

    /* check if mid path inode is directory */
    if(!(mid_inode->data).isDir)
      break;
  
    dir_close(dir);   // last mid_inode will be closed here
    dir = dir_open(mid_inode);

    if(token)
      free(token);
    token = next_token;
  }

fail:
  if(file_dir)
    *file_dir = NULL;
  if(file_name)
    *file_name = NULL;

done:
  inode_close(mid_inode);
  dir_close (dir);
  if(token)
    free(token);
  if(next_token)
    free(next_token);

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
  
  // TODO handle end slash??

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

bool
ch_dir(const char* path)
{
  struct inode *dir_inode = get_name_and_dir_from_path(path, NULL, NULL, 0);

  if(!dir_inode) {
    struct thread *cur_t = thread_current();
    dir_close(cur_t->working_directory);
    cur_t->working_directory = dir_open(dir_inode); // TODO Maybe update its children's working directories?
    
    return 1;
  }

  // inode_close(dir_inode); // Is it necessary?

  return 0;
}


bool
mk_dir(const char* path)
{
  struct inode *dir_inode = get_name_and_dir_from_path(path, NULL, NULL, 1);

  if(!dir_inode) 
    return 1;

  return 0;
}

bool
read_dir(struct file_descriptor* fd, void* buffer) {
  struct inode *fd_inode = fd->file->inode;
  
  if(!fd_inode)
    return 0;
  
  if(fd_inode->data.isDir == 1){
    struct dir *fd_dir = dir_open(fd_inode);

    /* I assume next comment should be done by user */

    // buffer = malloc((NAME_MAX + 1)*sizeof(char));

    bool out = dir_readdir(fd_dir, buffer);
    dir_close(fd_dir);

    return out;
  }

  return 0;

  /**
   * 
  struct dir_entry *e;

  while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e)
  {
    dir->pos += sizeof e;
      if (e.in_use)
        {
          dir->pos -= sizeof e;
          return e;
        }
  }
  return NULL;
  */

}

bool
is_dir(struct file_descriptor* fd) {

  struct inode *fd_inode = fd->file->inode;

  if(!fd_inode)
    return 0;
  
  if(fd_inode->data.isDir == 1)
    return 1;
  

  return 0;
}