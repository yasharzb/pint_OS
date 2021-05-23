#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include <stdbool.h>
#include "threads/synch.h"
#include "threads/fixed-point.h"
#include "filesys/file-descriptor.h"
#include "filesys/directory.h"


/* States in a thread's life cycle. */
enum thread_status
{
   THREAD_RUNNING, /* Running thread. */
   THREAD_READY,   /* Not running but ready to run. */
   THREAD_BLOCKED, /* Waiting for an event to trigger. */
   THREAD_DYING    /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t)-1) /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0          /* Lowest priority. */
#define PRI_DEFAULT 31     /* Default priority. */
#define PRI_MAX 63         /* Highest priority. */
#define INITIAL_FD_COUNT 2 /* Thread initial fd count */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
{
   /* Owned by thread.c. */
   tid_t tid;                 /* Thread identifier. */
   enum thread_status status; /* Thread state. */
   char name[16];             /* Name (for debugging purposes). */
   uint8_t *stack;            /* Saved stack pointer. */
   int priority;              /* Priority. */
   struct list_elem allelem;  /* List element for all threads list. */

   /* Shared between thread.c and synch.c. */
   struct list_elem elem; /* List element. */

// #ifdef USERPROG
   /* Owned by userprog/process.c. */
   uint32_t *pagedir; /* Page directory. */
// #endif

   /* Owned by thread.c. */
   unsigned magic; /* Detects stack overflow. */

   tid_t parent_tid; /* Parent thread tid */

   struct list children_list;   /* List of thread children */
   struct list_elem child_elem; /* List element for children_list */

   struct semaphore exited; /* Semaphore for knowing if thread exited in wait */
   int exit_value;          /* Exit value of process */

   bool wait_on_called;       /* True if parent already wait on process */
   struct semaphore can_free; /* Semaphore to know if thread struct can be freed */

   bool load_success_status;   /* Status of loading of executable file */
   struct semaphore load_done; /* Semaphore to know if executable has finished loading in exec */

   struct file *executable_file; /* Thread executable file */

   int fd_counter;      /* Thread current file descriptors count */
   struct list fd_list; /* Thread file descriptors list */

   int effective_priority;         /* Thread effective priority */
   struct list holding_locks_list; /* List of locks that thread is holding */
   struct lock *waiting_lock;      /* Lock that thread is waiting to acquire */
   
   int64_t target_ticks;         /* Tick at which thread must wake up if it is sleep */
   struct list_elem alarm_elem;  /* List elem for storing thread in sleep_threads_list */

   struct dir *working_directory; /* Thread working directory - NULL means root dir.
                                     Access it using `get_working_directory` function. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

bool cmp_target_ticks(const struct list_elem *a, const struct list_elem *b, void *aux);

void thread_init(void);
void thread_start(void);

void thread_tick(void);
void thread_print_stats(void);

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *, void *);

void thread_block(void);
void thread_unblock(struct thread *);

struct thread *thread_current(void);
tid_t thread_tid(void);
const char *thread_name(void);

void thread_exit(void) NO_RETURN;
void thread_yield(void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func(struct thread *t, void *aux);
void thread_foreach(thread_action_func *, void *);

int thread_get_priority(void);
void thread_set_priority(int);

int thread_get_nice(void);
void thread_set_nice(int);
int thread_get_recent_cpu(void);
int thread_get_load_avg(void);

struct thread *get_thread(tid_t tid);
struct thread *get_child_thread(tid_t child_tid);
void prepare_thread_for_exit(int exit_value);

bool thread_priority_less_function(const struct list_elem *a,
                                   const struct list_elem *b,
                                   void *aux);

struct thread *get_and_remove_next_thread(struct list *list);

void compare_priority_and_update(struct thread *t, int priority);
void calculate_priority_and_yield(struct thread *t);

void thread_yield_if_necessery(void);

struct dir* get_working_directory(void);

#endif /* threads/thread.h */
