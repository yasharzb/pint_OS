#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define MAX_SYSCALL_ARGS 4
#define MAX_SYSCALK_ARG_LENGTH 4095

// typedef struct global_file
// {
//     char *file_name;
//     struct semaphore sem;
//     struct list_elem gf_elem;
// } global_file;

void syscall_init(void);

#endif /* userprog/syscall.h */
