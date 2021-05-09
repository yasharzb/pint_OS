<<<<<<< HEAD
=======
#include "filesys/file-descriptor.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"

static struct lock rw_lock;
static struct lock fd_number_lock;

void file_descriptor_init()
{
  lock_init(&rw_lock);
  lock_init(&fd_number_lock);
}

static int
allocate_fd_number(void)
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
  lock_acquire(&rw_lock);

  bool success = filesys_remove(file_name);

  lock_release(&rw_lock);
  return success;
}

bool create_file(const char *name, off_t initial_size)
{
  lock_acquire(&rw_lock);

  bool success = filesys_create(name, initial_size);

  lock_release(&rw_lock);
  return success;
}

bool close_fd(int fd, bool remove_from_fd_list)
{
  lock_acquire(&rw_lock);

  bool success = false;
  fd = is_valid_fd(fd);
  if (fd != -1)
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)
    {
      file_close(f_file->file);
      if (remove_from_fd_list)
      {
        list_remove(&f_file->fd_elem);
        palloc_free_page(f_file);
      }
      success = true;
    }
  }

  lock_release(&rw_lock);
  return success;
}

file_descriptor *
create_file_descriptor(char *file_name, struct thread *cur_thread)
{
  lock_acquire(&rw_lock);

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

      list_push_back(&cur_thread->fd_list, &file_d->fd_elem);
    }
  }

  lock_release(&rw_lock);
  return file_d;
}

int fd_write(int fd, void *buffer, unsigned size)
{
  lock_acquire(&rw_lock);

  int w_bytes_cnt = -1;

  fd = is_valid_fd(fd);
  if (fd != -1)
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)
      w_bytes_cnt = file_write(f_file->file, buffer, size);
  }

  lock_release(&rw_lock);
  return w_bytes_cnt;
}

int fd_read(int fd, void *buffer, unsigned size)
{
  lock_acquire(&rw_lock);

  int read_bytes_cnt = -1;

  fd = is_valid_fd(fd);
  if (fd != -1)
  {
    file_descriptor *f_file = get_file_from_current_thread(fd);
    if (f_file != NULL)

      read_bytes_cnt = file_read(f_file->file, buffer, size);
  }

  lock_release(&rw_lock);
  return read_bytes_cnt;
}

int size_file(int fd)
{
  lock_acquire(&rw_lock);

  int size = -1;
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    size = (int)file_length(fd_tmp->file);

  lock_release(&rw_lock);
  return size;
}

void seek_file(int fd, unsigned position)
{
  lock_acquire(&rw_lock);

  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    file_seek(fd_tmp->file, position);

  lock_release(&rw_lock);
  return;
}

unsigned
tell_file(int fd)
{
  lock_acquire(&rw_lock);

  int tell = -1;
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    tell = file_tell(fd_tmp->file);

  lock_release(&rw_lock);
  return tell;
}
>>>>>>> parent of 42aab7c... Fix problem with thread yield in userprog tests
