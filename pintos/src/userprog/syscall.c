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

#include "userprog/exception.h"
#include "userprog/process.h"

#define MAX_FD 1025

static void syscall_handler(struct intr_frame *);
uint32_t *assign_args(uint32_t *esp);
void *copy_user_mem_to_kernel(void *src, int size, bool null_terminated);
void *get_kernel_va_for_user_pointer(void *ptr);
tid_t exec(const char *file_name);
bool create_file_descriptor(char *buffer, struct thread *cur_thread, file_descriptor *file_d);
int is_valid_fd(long *args);
char *find_file_name(int fd, struct list *fd_list, struct file *f_file);
void close_all(char *file_name, struct list *fd_list);

/* Static list to keep global_file_descs for thread safety*/
static struct list gf_list;

void syscall_init(void)
{
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    list_init(&gf_list);
}

static void
syscall_handler(struct intr_frame *f)
{
    bool success = true;
    void *buffer = NULL;
    struct thread *cur_thread;

    // TODO now this will try to copy 16 bytes from esp to kernel memory
    // (since syscalls has at most 3 arg) but it should be dynamic based
    // on syscall cause esp can be near end and can cause killing user
    // process when everything is good
    uint32_t *args = assign_args((uint32_t *)f->esp);

    if (args == NULL)
    {
        goto kill_process;
    }

    /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

    /* printf("System call number: %d\n", args[0]); */

    switch (args[0])
    {
    /* void exit (int status) */
    case SYS_EXIT:
        f->eax = args[1];
        // set thread exit value (for wait)
        prepare_thread_for_exit(args[1]);
        thread_exit();
        break;

    /* int write (int fd, const void *buffer, unsigned size) */
    case SYS_WRITE:
        // args[2] is a pointer so we need to validate it:
        buffer = get_kernel_va_for_user_pointer((void *)args[2]);
        if (buffer == NULL)
            goto kill_process;

        if (args[1] == STDOUT_FILENO)
        {
            putbuf((char *)buffer, args[3]);
        }
        // else
        // {
        //     int fd = is_valid_fd(args);
        //     if (fd == -1)
        //     {
        //         success = false;
        //         goto done;
        //     }
        //     cur_thread = thread_current();
        //     struct file *f_file;
        //     find_file_name(fd, &cur_thread->fd_list, f_file);
        //     f_file
        // }
        break;

    /* int practice (int i) */
    case SYS_PRACTICE:
        f->eax = args[1] + 1;
        break;

    /* pid_t exec (const char *cmd_line) */
    case SYS_EXEC:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
        {
            success = false;
            goto done;
        }

        tid_t child_tid = exec(buffer);
        f->eax = child_tid;
        break;

    /* int wait (pid_t pid) */
    case SYS_WAIT:
        f->eax = process_wait((tid_t)args[1]);
        break;


    case SYS_OPEN:
        if ((void *)args[1] == NULL)
        {
            success = false;
            goto done;
        }

        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if (buffer == NULL)
            goto kill_process;
        cur_thread = thread_current();
        file_descriptor *file_d = palloc_get_page(0);
        if (!create_file_descriptor((char *)buffer, cur_thread, file_d))
        {
            success = false;
            goto done;
        }
        f->eax = file_d->fd;
        break;

    case SYS_CLOSE:
        if ((void *)args[1] == NULL)
        {
            success = false;
            goto done;
        }
        int fd = is_valid_fd((long *)args);
        if (fd == -1)
        {
            success = false;
            goto done;
        }
        cur_thread = thread_current();
        struct file *f_file;
        char *file_name = find_file_name(fd, &cur_thread->fd_list, f_file);
        if (file_name != NULL)
            file_close(f_file);
        else
            success = false;
        break;


    case SYS_CREATE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if(buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = create_file(buffer, (unsigned) args[2]);
        break;

    case SYS_REMOVE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1]);
        if(buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = remove_file(buffer);
        break;


    case SYS_FILESIZE:
        f->eax = size_file((int) args[1]);
        break;

    case SYS_READ:
        break;

    case SYS_TELL:
        f->eax = tell_file((int) args[1]);
        break;

    case SYS_SEEK:
        seek_file((int)args[1], (unsigned)args[2]);
        break;
        
    default:
        break;
    }

done:

    palloc_free_page(args);
    palloc_free_page(buffer);

    if (!success)
        f->eax = -1;
    return;

kill_process:
    palloc_free_page(args);
    palloc_free_page(buffer);
    prepare_thread_for_exit(-1);
    thread_exit();
}

/**** exec syscall ****/

tid_t exec(const char *file_name)
{
    tid_t child_tid = process_execute(file_name);
    if (child_tid == TID_ERROR)
        goto done;
    struct thread *t = get_thread(child_tid);

    if (t == NULL)
    {
        child_tid = TID_ERROR;
        goto done;
    }
    if (!t->load_success_status)
    {
        child_tid = TID_ERROR;
        goto done;
    }

done:
    return child_tid;
}

/**** accessing user memory ****/

uint32_t *
assign_args(uint32_t *esp)
{
    /* check if stack pointer is not corrupted */
    if (!is_user_vaddr(esp))
        return false;

    /* copy arguments in user stack to kernel */
    char *buffer = copy_user_mem_to_kernel(esp, MAX_SYSCALL_ARGS * 4, false);
    if (buffer == NULL)
        return false;

    return (uint32_t *)buffer;
}

/* copy user memory pointed by `ptr` to kernel memory
   and return pointer to allocated address */
void *
get_kernel_va_for_user_pointer(void *ptr)
{
    if (ptr == NULL || !is_user_vaddr(ptr))
        return NULL;

    char *buffer = copy_user_mem_to_kernel(ptr, MAX_SYSCALK_ARG_LENGTH, true);
    if (buffer == NULL)
        return NULL;

    /* set last byte in page to zero so kernel don't die in case of not valid string */
    buffer[MAX_SYSCALK_ARG_LENGTH] = 0;
    return (void *)buffer;
}

/* copy from src in user virtual address to dst in kernel virtual
    address. if null_terminated it will continue until reaching zero,
    else it will copy `size` bytes. Max size can be PAGE_SIZE.
    (you can implement o.w. if you need.)
    will return NULL if fail. */
void *
copy_user_mem_to_kernel(void *src, int size, bool null_terminated)
{
    if (size > PGSIZE)
        return NULL;

    /* allocate a page to store data */
    char *buffer = palloc_get_page(0);
    if (buffer == NULL)
        goto fail;

    char *current_address = src;
    int copied_size = 0;
    while (copied_size < size)
    {
        if (!is_user_vaddr(current_address))
            goto fail;

        /* maximum size that we can copy in this page */
        int cur_size = (uintptr_t)pg_round_up((void *)current_address) - (uintptr_t)current_address + 1;
        if (size - copied_size < cur_size)
            cur_size = size - copied_size;

        /* get kernel virtual address corresponding to currrent address */
        char *kernel_address = pagedir_get_page(thread_current()->pagedir, (void *)current_address);
        if (kernel_address == NULL)
            goto fail;

        memcpy(buffer + copied_size, kernel_address, cur_size);

        /* we will search for null here if we need to copy until null */
        if (null_terminated)
        {
            char *temp = buffer + copied_size;
            bool null_found = false;
            while (temp < buffer + copied_size + cur_size)
            {
                if (!(*temp))
                    null_found = true;
                temp++;
            }
            // if null found stop copy.
            if (null_found)
                break;
        }

        copied_size += cur_size;
        current_address += cur_size;
    }

    return buffer;

    // end
fail:
    if (buffer)
        palloc_free_page(buffer);
    return NULL;
}

// void write_perm(char *file_name, struct file *f_file)
// {
//     struct list_elem *e;
//     global_file *gf;
//     for (e = list_begin(&gf_list); e != list_end(&gf_list); e = list_next(e))
//     {
//         gf = list_entry(e, global_file, gf_elem);
//         if (strcmp(gf->file_name, file_name) == 0)
//         {
//         }
//     }
// }

bool create_file_descriptor(char *buffer, struct thread *cur_thread, file_descriptor *file_d)
{
    if (file_d == NULL)
        return false;
    if (cur_thread->fd_counter == INITIAL_FD_COUNT)
        list_init(&cur_thread->fd_list);
    file_d->fd = cur_thread->fd_counter++;
    struct file *o_file = filesys_open(buffer);
    if (o_file == NULL)
        return false;
    file_d->file_name = buffer;
    file_d->file = o_file;
    list_push_back(&cur_thread->fd_list, &file_d->fd_elem);
    return true;
}

int is_valid_fd(long *args)
{
    long l_fd = args[1];
    if (l_fd > MAX_FD)
        return -1;
    if (l_fd < INITIAL_FD_COUNT)
        return -1;
    return (int)(l_fd);
}

char *find_file_name(int fd, struct list *fd_list, struct file *f_file)
{
    struct list_elem *e;
    file_descriptor *file_d;
    for (e = list_begin(fd_list); e != list_end(fd_list); e = list_next(e))
    {
        file_d = list_entry(e, file_descriptor, fd_elem);
        if (file_d->fd == fd)
        {
            f_file = file_d->file;
            return file_d->file_name;
        }
    }
    return NULL;
}

void close_all(char *file_name, struct list *fd_list)
{
    struct list_elem *e;
    file_descriptor *file_d;
    for (e = list_begin(fd_list); e != list_end(fd_list); e = list_next(e))
    {
        file_d = list_entry(e, file_descriptor, fd_elem);
        if (strcmp(file_d->file_name, file_name) == 0)
        {
            list_remove(&file_d->fd_elem);
            file_close(file_d->file);
        }
    }
}