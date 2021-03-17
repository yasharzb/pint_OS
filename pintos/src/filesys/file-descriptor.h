#ifndef FILESYS_FILE_DESCRIPTOR_H
#define FILESYS_FILE_DESCRIPTOR_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "filesys/off_t.h"
#include "threads/thread.h"

typedef struct file_descriptor
{

   struct file *file;
   char *file_name;
   int fd;
   bool closed;
   bool removed;
   struct list_elem fd_elem;
} file_descriptor;


bool remove_file(const char *fn);
bool create_file(const char *name, off_t initial_size);
int size_file(int fd);

#endif /* filesys/file-descriptor.h */
