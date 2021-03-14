#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "threads/palloc.h"

static void syscall_handler(struct intr_frame *);
uint32_t *assign_args(uint32_t *esp);
void *copy_user_mem_to_kernel(void *src, int size);
void *get_kernel_va_for_user_pointer(void *ptr);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  uint32_t *args = assign_args((uint32_t *)f->esp);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  /* printf("System call number: %d\n", args[0]); */

  if (args[0] == SYS_EXIT)
  {
    f->eax = args[1];
    printf("%s: exit(%d)\n", &thread_current()->name, args[1]);
    thread_exit();
  }

  if (args[0] == SYS_WRITE) //int write (int fd, const void *buffer, unsigned size)
  {
    // args[2] is a pointer so we need to validate it:
    args[2] = (uint32_t *)get_kernel_va_for_user_pointer((void *)args[2]);
    
    if (args[1] == STDOUT_FILENO)
    {
      putbuf((char *)args[2], args[3]);
    }
  }
}

uint32_t *
assign_args(uint32_t *esp)
{
  /* check if stack pointer is not corrupted */
  if (!is_user_vaddr(esp))
    return false;

  /* copy arguments in user stack to kernel */
  char *buffer = copy_user_mem_to_kernel(esp, MAX_SYSCALL_ARGS * 4);
  if (buffer == NULL)
    return false;

  return (uint32_t *)buffer;
}

/* copy user memory pointed by `ptr` to kernel memory
   and return pointer to allocated address */
void *
get_kernel_va_for_user_pointer(void *ptr)
{
  char *buffer = copy_user_mem_to_kernel(ptr, MAX_SYSCALK_ARG_LENGTH);
  if (buffer == NULL)
    return NULL;

  /* set last byte in page to zero so kernel don't die in case of not valid string */
  buffer[MAX_SYSCALK_ARG_LENGTH] = 0;
  return (void *)buffer;
}

/* copy from src in user virtual address to dst in kernel virtual
    address for size bytes. Max size can be PAGE_SIZE.
    (you can implement o.w. if you need.)
    will return NULL if fail. */
void *
copy_user_mem_to_kernel(void *src, int size)
{
  if (size > PGSIZE)
    return NULL;

  /* get kernel virtual address corresponding to src */
  char *kernel_ptr = pagedir_get_page(thread_current()->pagedir, src);
  if (kernel_ptr == NULL)
    return NULL;

  /* allocate a page to store data */
  char *buffer = palloc_get_page(0);
  if (buffer == NULL)
    return NULL;

  char *current_address = src;
  int copied_size = 0;
  while (copied_size < size)
  {
    /* maximum size that we can copy in this page */
    int cur_size = (uintptr_t)pg_round_up((void *)current_address) - (uintptr_t)current_address + 1;
    if (size - copied_size < cur_size)
      cur_size = size - copied_size;
    
    memcpy(buffer + copied_size, current_address, cur_size);
    copied_size += cur_size;
    current_address += cur_size;
  }

  return buffer;
}
