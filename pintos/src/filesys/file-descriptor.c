#include "filesys/file-descriptor.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "devices/input.h"
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

/* Returns a tid to use for a new thread. */
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
  return filesys_remove(file_name);
  // if(successful){
  //   struct list_elem *e;
  //   for (e = list_begin(&all_list); e != list_end(&all_list);
  //      e = list_next(e))
  //   {
  //     struct thread *t = list_entry(e, struct thread, allelem);
  //     struct list_elem *el;
  //     for (el = list_begin(&(t->fd_list)); el != list_end(&(t->fd_list));
  //           el = list_next(el))
  //     {
  //       struct file_descriptor *fd = list_entry(el, struct file_descriptor, fd_elem);
  //       if(strcmp(fd->file_name, fn) == 0 )
  //       {
  //         fd->removed = 1;
  //       }
  //     }
  //   }
  // }
}

bool create_file(const char *name, off_t initial_size)
{
  return filesys_create(name, initial_size);
}

bool close_fd(int fd)
{
  fd = is_valid_fd(fd);
  if (fd == -1)
  {
    return false;
  }
  file_descriptor *f_file = get_file_from_current_thread(fd);
  if (f_file != NULL)
  {
    file_close(f_file->file);
    list_remove(&f_file->fd_elem);
    return true;
  }
  return false;
}

file_descriptor *
create_file_descriptor(char *file_name, struct thread *cur_thread)
{
  file_descriptor *file_d = palloc_get_page(0);
  if (file_d == NULL)
    return NULL;
  
  struct file *o_file = filesys_open(file_name);
  if (o_file == NULL)
    return NULL;
    
  file_d->fd = allocate_fd_number();
  file_d->file_name = file_name;
  file_d->file = o_file;

  list_push_back(&cur_thread->fd_list, &file_d->fd_elem);

  return file_d;
}

int fd_write(int fd, void *buffer, unsigned size)
{
  lock_acquire(&rw_lock);

  int w_bytes_cnt = -1;
  switch (fd)
  {
  case STDIN_FILENO:
    break;
  case STDOUT_FILENO:
    putbuf((char *)buffer, size);
    w_bytes_cnt = 0;
    break;
  default:
    fd = is_valid_fd(fd);
    if (fd != -1)
    {
      file_descriptor *f_file = get_file_from_current_thread(fd);
      if (f_file != NULL)
        w_bytes_cnt = file_write(f_file->file, buffer, size);
    }
    break;
  }

  lock_release(&rw_lock);
  return w_bytes_cnt;
}

int fd_read(int fd, void *buffer, unsigned size)
{
  lock_acquire(&rw_lock);

  int read_bytes_cnt = -1;
  switch (fd)
  {
  case STDIN_FILENO:
    read_bytes_cnt = input_getc();
    break;
  case STDOUT_FILENO:
    break;
  default:
    fd = is_valid_fd(fd);
    if (fd != -1)
    {
      file_descriptor *f_file = get_file_from_current_thread(fd);
      if (f_file != NULL)

        read_bytes_cnt = file_read(f_file->file, buffer, size);
    }
    break;
  }
  lock_release(&rw_lock);
  return read_bytes_cnt;
}

int size_file(int fd)
{
  struct file_descriptor *fd_tmp = get_file_from_current_thread(fd);
  if (fd_tmp)
    return (int)file_length(fd_tmp->file);
  return -1;
}

void seek_file(int fd, unsigned position)
{
  struct list_elem *el;
  struct thread *t = thread_current();
  for (el = list_begin(&(t->fd_list)); el != list_end(&(t->fd_list));
       el = list_next(el))
  {
    struct file_descriptor *fd_tmp = list_entry(el, struct file_descriptor, fd_elem);
    if (fd_tmp->fd == fd)
    {
      file_seek(fd_tmp->file, position);
      return;
    }
  }
  return;
}

unsigned
tell_file(int fd)
{
  struct list_elem *el;
  struct thread *t = thread_current();
  for (el = list_begin(&(t->fd_list)); el != list_end(&(t->fd_list));
       el = list_next(el))
  {
    struct file_descriptor *fd_tmp = list_entry(el, struct file_descriptor, fd_elem);
    if (fd_tmp->fd == fd)
    {
      return file_tell(fd_tmp->file);
    }
  }
  //this fd doesn't exist
  return -1;
}
