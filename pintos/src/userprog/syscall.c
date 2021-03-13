#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}




// our code

void exec(const char* file_name) {
  tid_t child_tid = process_execute(file_name);
  struct thread *t = get_thread(child_tid);

  ASSERT(t != NULL);
  sema_down(&t->load_done);
  if (t->load_success_status) {
    //printf("failed to fork a new process\n");
  } else {
    //printf("new process forked successfully\n");
  }
}

int _wait(tid_t child_tid) {
  return process_wait(child_tid);
}

// end



static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

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
      printf ("%s: exit(%d)\n", &thread_current ()->name, args[1]);

      // our code
      thread_exit_value(args[1]);
      // end

      // thread_exit ();
    }

  //TODO Add syscalls

 // our code
  if (args[0] == SYS_EXEC) {
    exec(args[1]);
    struct thread *t = thread_current();
  }

  if (args[0] == SYS_WAIT) {
    f->eax = _wait((tid_t) args[1]);
  }

  // end
}
