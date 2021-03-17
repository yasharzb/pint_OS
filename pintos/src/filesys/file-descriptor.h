#ifndef FILESYS_FILE_DESCRIPTOR_H
#define FILESYS_FILE_DESCRIPTOR_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "filesys/off_t.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

#define MAX_FD 1025

typedef struct file_descriptor
{

   struct file *file;
   char *file_name;
   int fd;
   bool closed;
   bool removed;
   struct list_elem fd_elem;
} file_descriptor;

void file_descriptor_init(void);

file_descriptor *get_file_from_current_thread(int fd);
int is_valid_fd(int fd);
bool remove_file(const char *fn);
bool create_file(const char *name, off_t initial_size);
int size_file(int fd);
bool close_fd(int fd);
void seek_file(int fd, unsigned position);
unsigned tell_file(int fd);

file_descriptor *create_file_descriptor(char *file_name, struct thread *cur_thread);
int fd_write(int fd, void *buffer, unsigned size);
int fd_read(int fd, void *buffer, unsigned size);

#endif /* filesys/file-descriptor.h */
