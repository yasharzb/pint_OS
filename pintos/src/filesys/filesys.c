#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"

bool filesys_create_name (char *name, off_t initial_size, struct dir *dir, bool isDir);
struct file *filesys_open_name (const char *name);
bool filesys_remove_name (char *name, struct dir* dir);
bool is_root_path(const char *path);

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
  
  if (!success && inode_sector != 0) {
    free_map_release (inode_sector, 1);
    goto done;
  }
  
  if(isDir) {
    struct dir *new_dir = dir_open(inode_open(inode_sector));
    success &= dir_add(new_dir, ".", inode_sector);
    success &= dir_add(new_dir, "..", dir->inode->sector);
    dir_close(new_dir);

    if(!success) {
      // dir_remove(new_dir, ".");
      // dir_remove(new_dir, "..");
      dir_remove(dir, name);
    }
  }


done: 
  dir_close (dir);
  free(name);

  return success;
}

bool
filesys_create (const char *path, off_t initial_size, bool isDir)
{
  struct dir* dir;
  char *name;

  struct inode *inode = get_name_and_dir_from_path(path, &name, &dir);
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
  struct dir *dir = dir_open_root (); // TODO should we always look for it from root? Saba: this
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
  struct inode *inode = get_name_and_dir_from_path(path, &name, &dir);

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

  struct dir *new_dir = dir_open(inode_open(ROOT_DIR_SECTOR));
  bool success = dir_add(new_dir, ".", ROOT_DIR_SECTOR) &&
    dir_add(new_dir, "..", ROOT_DIR_SECTOR);

  dir_close(new_dir);

  if(!success)
      PANIC ("root directory addition failed");

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
get_name_and_dir_from_path(const char *path, char **file_name, struct dir **file_dir)
{   
  /* special case! : root */
  if(is_root_path(path)) {
    if(file_dir)
      *file_dir = dir_open_root();
    if(file_name) {
      *file_name = malloc(2*sizeof(char));
      **file_name = '/';
      **file_name = 0;
    }
    return inode_open (ROOT_DIR_SECTOR);
  }

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
        inode = mid_inode;
      
      if(file_name) {
        *file_name = malloc(NAME_MAX + 1);
        memcpy(*file_name, token, strlen(token) + 1);
      }

      if(file_dir)
        *file_dir = dir_reopen(dir);

      goto done;
    }

    /* if token it's not last token and it's not found in dir then search is failed */
    if(!found)
      break;

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

  inode_close(mid_inode);

done:
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
  while(*path && (*path) != '/') 
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

/* check if path curresponds to root ie /, //, ////////, ... */
bool
is_root_path(const char *path)  {
  if(strlen(path) == 0)
    return false;

  int i = 0;
  while(*(path + i) && *(path + i) == '/')
    i++;

  if(!*(path + i))
    return true;
  return false;
}
