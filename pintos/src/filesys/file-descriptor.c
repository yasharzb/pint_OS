#include "filesys/file-descriptor.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"

static struct lock fd_number_lock;

static int allocate_fd_number(void);

void file_descriptor_init()
{
  lock_init(&fd_number_lock);
}

static int
allocate_fd_number()
{
  int fd;

  lock_acquire(&fd_number_lock);
  fd = thread_current()->fd_counter++;
  lock_release(&fd_number_lock);

  return fd;
}

int is_valid_fd(int fd)
{
  if (fd > MAX_FD)
    return -1;
  if (fd < INITIAL_FD_COUNT)
    return -1;
  return (int)(fd);
}

file_descriptor *
get_file_from_current_thread(int fd)
{
  struct thread *t = thread_current();
  struct list_elem *e;
  file_descriptor *file_d;
  for (e = list_begin(&t->fd_list); e != list_end(&t->fd_list); e = list_next(e))
  {
    file_d = list_entry(e, file_descriptor, fd_elem);
    if (file_d->fd == fd)
      return file_d;
  }
  return NULL;
}

bool remove_file(const char *file_name)
{
  bool success = filesys_remove(file_name);
  return success;
}

bool create_file(const char *name, off_t initial_size)
{
  bool success = filesys_create(name, initial_size, 0);
  return success;
}

bool close_fd(int fd, bool remove_from_fd_list)
{

  bool success = false;
  fd = is_valid_fd(fd);
  if (fd != -1)
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)
    {
      file_close(f_file->file);
      if (f_file->dir)
        dir_close(f_file->dir);

      if (remove_from_fd_list)
      {
        list_remove(&f_file->fd_elem);
        palloc_free_page(f_file);
      }
      success = true;
    }
  }

  return success;
}

file_descriptor *
create_file_descriptor(char *file_name, struct thread *cur_thread)
{

  file_descriptor *file_d = NULL;

  struct file *o_file = filesys_open(file_name);
  if (o_file != NULL)
  {
    file_d = palloc_get_page(0);
    if (file_d != NULL)
    {
      file_d->fd = allocate_fd_number();
      file_d->file_name = file_name;
      file_d->file = o_file;

      file_d->dir = NULL;
      if (o_file->inode->data.isDir)
      {
        file_d->dir = dir_open(inode_reopen(o_file->inode));
      }

      list_push_back(&cur_thread->fd_list, &file_d->fd_elem);
    }
  }

  return file_d;
}

int fd_write(int fd, void *buffer, unsigned size)
{
  int w_bytes_cnt = -1;

  fd = is_valid_fd(fd);
  if (fd != -1 && !fd_isdir(fd))
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)
      w_bytes_cnt = file_write(f_file->file, buffer, size);
  }

  return w_bytes_cnt;
}

int fd_read(int fd, void *buffer, unsigned size, bool with_ahead)
{
  int read_bytes_cnt = -1;

  fd = is_valid_fd(fd);
  if (fd != -1 && !fd_isdir(fd))
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)
      read_bytes_cnt = file_read(f_file->file, buffer, size, with_ahead);
  }

  return read_bytes_cnt;
}

int size_file(int fd)
{
  int size = -1;
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    size = (int)file_length(fd_tmp->file);

  return size;
}

void seek_file(int fd, unsigned position)
{
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    file_seek(fd_tmp->file, position);

  return;
}

unsigned
tell_file(int fd)
{
  int tell = -1;
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    tell = file_tell(fd_tmp->file);

  return tell;
}

bool fd_readdir(int fd, void *buffer)
{
  bool situation = 0;

  fd = is_valid_fd(fd);
  if (fd != -1)
  {

    file_descriptor *f = get_file_from_current_thread(fd);

    if (f != NULL)
      situation = read_dir(f, buffer);
  }

  return situation;
}

bool fd_isdir(int fd)
{
  fd = is_valid_fd(fd);
  if (fd == -1)
    return 0;

  file_descriptor *f = get_file_from_current_thread(fd);
  return f != NULL && f->dir != NULL;
}

bool ch_dir(const char *path)
{
  struct inode *dir_inode = get_name_and_dir_from_path(path, NULL, NULL);

  if (!dir_inode || !dir_inode->data.isDir)
    return 0;

  struct dir *old_dir = get_working_directory();
  struct thread *cur_t = thread_current();
  cur_t->working_directory = dir_open(dir_inode);
  dir_close(old_dir);
  return 1;
}

bool mk_dir(const char *path)
{
  return filesys_create(path, 0, 1);
}

bool read_dir(struct file_descriptor *fd, void *buffer)
{
  if (fd->dir != NULL)
  {
    struct dir *fd_dir = fd->dir;
    bool out = dir_readdir(fd_dir, buffer);
    return out;
  }

  return 0;
}

int fd_get_inumber(int fd)
{
  fd = is_valid_fd(fd);
  if (fd == -1)
    return -1;

  file_descriptor *f = get_file_from_current_thread(fd);
  return f->file->inode->sector;
}
