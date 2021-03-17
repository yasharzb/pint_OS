#include "filesys/file-descriptor.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>

/* file descriptors */

struct file *
get_file_from_fd(int fd)
{
//   struct thread *cur = running_thread();
  //TODO
}

bool
remove_file(const char *fn)
{
    bool successful = filesys_remove(fn);
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
    return successful;
}

bool
create_file(const char *name, off_t initial_size)
{
  return filesys_create(name, initial_size);
}

int
size_file(int fd)
{
  struct list_elem *el;
  struct thread *t = thread_current();

  for (el = list_begin(&(t->fd_list)); el != list_end(&(t->fd_list));
       el = list_next(el))
  {
    struct file_descriptor *fd_tmp = list_entry(el, struct file_descriptor, fd_elem);
    if(fd_tmp->fd == fd)
    {
      return (int)file_length (fd_tmp->file);
    }
  }
  
}
