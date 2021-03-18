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
#include "filesys/file-descriptor.h"
#include "devices/input.h"

static void syscall_handler(struct intr_frame *);

/* accessing user memory */
uint32_t *assign_args(uint32_t *esp);
void *copy_user_mem_to_kernel(void *src, int size, bool null_terminated);
void *get_kernel_va_for_user_pointer(void *ptr, int size);
bool validate_user_pointer(void *ptr, int size);

/**** process control syscalls ****/
tid_t exec(const char *file_name);

/* Static list to keep global_file_descs for thread safety*/
void syscall_init(void)
{
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    file_descriptor_init();
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

    /* int practice (int i) */
    case SYS_PRACTICE:
        f->eax = args[1] + 1;
        break;

    /* pid_t exec (const char *cmd_line) */
    case SYS_EXEC:
        buffer = get_kernel_va_for_user_pointer((void *)args[1], -1);
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

    /* int open (const char *file) */
    case SYS_OPEN:
        buffer = get_kernel_va_for_user_pointer((void *)args[1], -1);
        if (buffer == NULL)
            goto kill_process;
        cur_thread = thread_current();
        file_descriptor *file_d = create_file_descriptor((char *)buffer, cur_thread);
        if (file_d == NULL)
        {
            success = false;
            goto done;
        }
        f->eax = file_d->fd;
        break;

    /* void close (int fd) */
    case SYS_CLOSE:
        success = close_fd(args[1]);
        break;

    case SYS_CREATE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1], -1);
        if (buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = create_file(buffer, (unsigned)args[2]);
        break;

    case SYS_REMOVE:
        buffer = get_kernel_va_for_user_pointer((void *)args[1], -1);
        if (buffer == NULL)
        {
            success = false;
            goto kill_process;
        }

        f->eax = remove_file(buffer);
        break;

    case SYS_FILESIZE:
        f->eax = size_file((int)args[1]);
        break;

    /* int write (int fd, const void *buffer, unsigned size) */
    case SYS_WRITE:;
        // args[2] is a pointer so we need to validate it:
        unsigned w_size = args[3];
        buffer = get_kernel_va_for_user_pointer((void *)args[2], w_size);
        if (buffer == NULL)
            goto kill_process;

        int w_bytes_cnt = -1;
        switch (args[1])
        {
        case STDIN_FILENO:
            break;
        case STDOUT_FILENO:
            putbuf((char *)buffer, w_size);
            w_bytes_cnt = 0;
            break;
        default:
            w_bytes_cnt = fd_write(args[1], buffer, w_size);
            break;
        }
        f->eax = w_bytes_cnt;
        break;

    case SYS_READ:;
        unsigned read_size = args[3];

        if (!validate_user_pointer((void *)args[2], read_size))
            goto kill_process;

        int read_bytes_cnt = -1;
        switch (args[1])
        {
        case STDIN_FILENO:
            read_bytes_cnt = input_getc();
            break;
        case STDOUT_FILENO:
            break;
        default:
            read_bytes_cnt = fd_read(args[1], (void *)args[2], read_size);
            break;
        }
        f->eax = read_bytes_cnt;
        break;

    case SYS_TELL:
        f->eax = tell_file((int)args[1]);
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

/**** process control syscalls ****/

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
   and return pointer to allocated address.
   pass size=-1 if you don't know the size
   and it will defaulted to `MAX_SYSCALK_ARG_LENGTH`
   and will be null_terminated. */
void *
get_kernel_va_for_user_pointer(void *ptr, int size)
{
    bool null_terminated = false;
    if (size == -1)
    {
        size = MAX_SYSCALK_ARG_LENGTH;
        null_terminated = true;
    }

    if (ptr == NULL || !is_user_vaddr(ptr))
        return NULL;

    char *buffer = copy_user_mem_to_kernel(ptr, size, null_terminated);
    if (buffer == NULL)
        return NULL;

    if (null_terminated)
    {
        /* set last byte in page to zero so kernel don't die in case of not valid string */
        buffer[MAX_SYSCALK_ARG_LENGTH] = 0;
    }
    return (void *)buffer;
}

/* will check if memory from ptr to ptr+size is valid user memory */
bool validate_user_pointer(void *ptr, int size)
{
    if (ptr == NULL || !is_user_vaddr(ptr))
        return false;

    char *current_address = ptr;
    int seen_size = 0;
    while (seen_size < size)
    {
        if (!is_user_vaddr(current_address))
            return false;

        /* size remaining in this page */
        int cur_size = (uintptr_t)pg_round_up((void *)current_address) - (uintptr_t)current_address + 1;
        if (size - seen_size < cur_size)
            cur_size = size - seen_size;

        /* get kernel virtual address corresponding to currrent address */
        char *kernel_address = pagedir_get_page(thread_current()->pagedir, (void *)current_address);
        if (kernel_address == NULL)
            return false;

        seen_size += cur_size;
        current_address += cur_size;
    }

    return true;
}

/* copy from src in user virtual address to dst in kernel virtual
    address. if null_terminated it will continue until reaching zero,
    else it will copy `size` bytes. */
void *
copy_user_mem_to_kernel(void *src, int size, bool null_terminated)
{
    int number_of_required_pages = (size + PGSIZE - 1) / PGSIZE;

    // number need to be at least one so it won't return null if
    // size == 0
    if (number_of_required_pages == 0)
        number_of_required_pages = 1;

    /* allocate a page to store data */
    char *buffer = palloc_get_multiple(0, number_of_required_pages);
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

fail:
    if (buffer)
        palloc_free_page(buffer);
    return NULL;
}
