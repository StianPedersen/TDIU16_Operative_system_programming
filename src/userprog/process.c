#include <debug.h>
#include <stdio.h>
#include <string.h>

#include "userprog/gdt.h"      /* SEL_* constants */
#include "userprog/process.h"
#include "userprog/load.h"
#include "userprog/pagedir.h"  /* pagedir_activate etc. */
#include "userprog/tss.h"      /* tss_update */
#include "filesys/file.h"
#include "threads/flags.h"     /* FLAG_* constants */
#include "threads/thread.h"
#include "threads/vaddr.h"     /* PHYS_BASE */
#include "threads/interrupt.h" /* if_ */
#include "threads/init.h"      /* power_off() */
#include "lib/string.h"

/* Headers not yet used that you may need for various reasons. */
#include "threads/synch.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"

#include "userprog/flist.h"
#include "userprog/plist.h"
/* HACK defines code you must remove and implement in a proper way */
#define HACK
#define STACK_DEBUG(...) printf(__VA_ARGS__)

/**************************OUR FUNCTIONS*********************************/

struct main_args
{
  /* Hint: When try to interpret C-declarations, read from right to
   * left! It is often easier to get the correct interpretation,
   * altough it does not always work. */

  /* Variable "ret" that stores address (*ret) to a function taking no
   * parameters (void) and returning nothing. */
  void (*ret)(void);

  /* Just a normal integer. */
  int argc;

  /* Variable "argv" that stores address to an address storing char.
   * That is: argv is a pointer to char*
   */
  char** argv;
};

void dump(void* ptr, int size)
{
  int i;
  const int S = sizeof(void*);

  printf("%2$-*1$s \t%3$-*1$s \t%4$-*1$s\n", S*2, "Address", "hex-data", "char-data");

  for (i = size - 1; i >= 0; --i)
  {
    void** adr = (void**)((unsigned long)ptr + i);
    unsigned char* byte = (unsigned char*)((unsigned long)ptr + i);

    printf("%0*lx\t", S*2, (unsigned long)ptr + i); /* address */

    if ((i % S) == 0)
      /* seems we're actually forbidden to read unaligned adresses */
      printf("%0*lx\t", S*2, (unsigned long)*adr); /* content interpreted as address */
    else
      printf("%*c\t", S*2, ' '); /* fill */

    if(*byte >= 32 && *byte < 127)
      printf("%c\n", *byte); /* content interpreted as character */
    else
      printf("\\%o\n", *byte);

    if ((i % S) == 0)
      printf("# ----------------------------------------------------------------\n");
  }
}

bool exists_in(char c, const char* d)
{
  int i = 0;
  while (d[i] != '\0' && d[i] != c)
    ++i;
  return (d[i] == c);
}

int count_args(const char* buf, const char* delimeters)
{
  int i = 0;
  bool prev_was_delim;
  bool cur_is_delim = true;
  int argc = 0;

  while (buf[i] != '\0')
  {
    prev_was_delim = cur_is_delim;
    cur_is_delim = exists_in(buf[i], delimeters);
    argc += (prev_was_delim && !cur_is_delim);
    ++i;
  }
  return argc;
}

void* setup_main_stack(const char* command_line, void* stack_top)
{
  /* Variable "esp" stores an address, and at the memory loaction
   * pointed out by that address a "struct main_args" is found.
   * That is: "esp" is a pointer to "struct main_args" */
  struct main_args* esp;
  int argc;
  int total_size;
  int line_size;
  // int cmdl_size;

  /* "cmd_line_on_stack" and "ptr_save" are variables that each store
   * one address, and at that address (the first) char (of a possible
   * sequence) can be found. */
  char* cmd_line_on_stack;
  char* ptr_save;
  int i = 0;


  /* calculate the bytes needed to store the command_line */
  line_size = strlen(command_line)+1;
  // STACK_DEBUG("# line_size = %d\n", line_size);

  /* round up to make it even divisible by 4 */
  int rounding_number = line_size%4;
  line_size = line_size + rounding_number;
  // STACK_DEBUG("# line_size (aligned) = %d\n", line_size);

  /* calculate how many words the command_line contain */
  argc = count_args(command_line, " ");
  // STACK_DEBUG("# argc = %d\n", argc);

  /* calculate the size needed on our simulated stack */
  total_size = (argc * sizeof(char*)) + line_size + sizeof(char**) + sizeof(int) + sizeof(void(*)(void));
  // STACK_DEBUG("# total_size = %d\n", total_size);

  /* calculate where the final stack top will be located */
  esp = (struct main_args*)((unsigned*)stack_top-total_size);
  /* setup return address and argument count */
  esp->ret = 0;
  esp->argc = argc;
  /* calculate where in the memory the argv array starts */
  esp->argv = (char**)esp + 3;

  /* calculate where in the memory the words is stored */
  cmd_line_on_stack = (char*)(esp->argv + argc+1);


  /* copy the command_line to where it should be in the stack */
  //cmdl_size?
    strlcpy(cmd_line_on_stack, command_line, line_size); //STRLCOPY??????

  /* build argv array and insert null-characters after each word */


  for (char* token = strtok_r(cmd_line_on_stack," ", &ptr_save);
   token != NULL; token = strtok_r(NULL, " ", &ptr_save))
    {
      esp->argv[i] = token;
      i++;
    }

  return esp; /* the new stack top */
}

/***********************************END*************************************/

/* This function is called at boot time (threads/init.c) to initialize
 * the process subsystem. */
 struct process_list plist;
void process_init(void)
{
  plist_init(&plist);
  struct thread *cur = thread_current ();
  plist_insert(&plist, cur->tid, 0, cur->name);
}

/* This function is currently never called. As thread_exit does not
 * have an exit status parameter, this could be used to handle that
 * instead. Note however that all cleanup after a process must be done
 * in process_cleanup, and that process_cleanup are already called
 * from thread_exit - do not call cleanup twice! */
void process_exit(int status)
{
  struct running_process* cur = plist_find(&plist, thread_current()->tid);
  cur->exit_code = status;
  thread_exit();
}

/* Print a list of all running processes. The list shall include all
 * relevant debug information in a clean, readable format. */
void process_print_list()
{
 print_list(&plist);
}


struct parameters_to_start_process
{
  char* command_line;
  struct semaphore oursema;
  int P_id;
  int parent_id;
};

static void
start_process(struct parameters_to_start_process* parameters) NO_RETURN;

/* Starts a new proccess by creating a new thread to run it. The
   process is loaded from the file specified in the COMMAND_LINE and
   started with the arguments on the COMMAND_LINE. The new thread may
   be scheduled (and may even exit) before process_execute() returns.
   Returns the new process's thread id, or TID_ERROR if the thread
   cannot be created. */
int
process_execute (const char *command_line)
{
  char debug_name[64];
  int command_line_size = strlen(command_line) + 1;
  tid_t thread_id = -1;
  int  process_id = -1;

  /* LOCAL variable will cease existence when function return! */
  struct parameters_to_start_process arguments;

  debug("%s#%d: process_execute(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        command_line);

  /* COPY command line out of parent process memory */
  arguments.command_line = malloc(command_line_size);
  strlcpy(arguments.command_line, command_line, command_line_size);


  strlcpy_first_word (debug_name, command_line, 64);

  /*INIT semaphore */
  sema_init(&arguments.oursema,0);
  /* SCHEDULES function `start_process' to run (LATER) */
  arguments.parent_id=thread_current()->tid;
  thread_id = thread_create (debug_name, PRI_DEFAULT,
                             (thread_func*)start_process, &arguments);

  //process_id = thread_id;
  arguments.P_id = -1;

  if(thread_id != -1)
  {
    /*Does sema_down because waiting for stack to be completed*/
    sema_down(&arguments.oursema);
    process_id=arguments.P_id;
  }

  /* WHICH thread may still be using this right now? */
  free(arguments.command_line);

  debug("%s#%d: process_execute(\"%s\") RETURNS %d\n",
        thread_current()->name,
        thread_current()->tid,
        command_line, process_id);

  /* MUST be -1 if `load' in `start_process' return false */

  return process_id;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (struct parameters_to_start_process* parameters)
{
  /* The last argument passed to thread_create is received here... */
  struct intr_frame if_;
  bool success;

  char file_name[64];
  strlcpy_first_word (file_name, parameters->command_line, 64);

  debug("%s#%d: start_process(\"%s\") ENTERED\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;

  success = load (file_name, &if_.eip, &if_.esp);

  debug("%s#%d: start_process(...): load returned %d\n",
        thread_current()->name,
        thread_current()->tid,
        success);
  if (success)
  {
    /* We managed to load the new program to a process, and have
       allocated memory for a process stack. The stack top is in
       if_.esp, now we must prepare and place the arguments to main on
       the stack. */

    /* A temporary solution is to modify the stack pointer to
       "pretend" the arguments are present on the stack. A normal
       C-function expects the stack to contain, in order, the return
       address, the first argument, the second argument etc. */

        if_.esp = setup_main_stack(parameters->command_line, if_.esp);

    /* The stack and stack pointer should be setup correct just before
       the process start, so this is the place to dump stack content
       for debug purposes. Disable the dump when it works. */

     // dump_stack ( PHYS_BASE + 15, PHYS_BASE - if_.esp + 16 );
     //LÄGG TILL PROCESS I LISTAN
     plist_insert(&plist, thread_current()->tid, parameters->parent_id, thread_current()->name);

     parameters->P_id = thread_current()->tid;
  }

  debug("%s#%d: start_process(\"%s\") DONE\n",
        thread_current()->name,
        thread_current()->tid,
        parameters->command_line);

  sema_up(&parameters->oursema);


  /* If load fail, quit. Load may fail for several reasons.
     Some simple examples:
     - File doeas not exist
     - File do not contain a valid program
     - Not enough memory
  */
  if ( ! success )
  {
    thread_exit ();
  }


  /* Start the user process by simulating a return from an interrupt,
     implemented by intr_exit (in threads/intr-stubs.S). Because
     intr_exit takes all of its arguments on the stack in the form of
     a `struct intr_frame', we just point the stack pointer (%esp) to
     our stack frame and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Wait for process `child_id' to die and then return its exit
   status. If it was terminated by the kernel (i.e. killed due to an
   exception), return -1. If `child_id' is invalid or if it was not a
   child of the calling process, or if process_wait() has already been
   successfully called for the given `child_id', return -1
   immediately, without waiting.

   This function will be implemented last, after a communication
   mechanism between parent and child is established. */
int
process_wait (int child_id)
{
  int status = -1;
  struct thread *cur = thread_current ();
  debug("%s#%d: process_wait(%d) ENTERED\n",
        cur->name, cur->tid, child_id);
  /* Yes! You need to do something good here ! */
  struct running_process* child = plist_find(&plist, child_id);
  // struct running_process* parent = plist_find(&plist, cur->tid);
  if(child == NULL)
  {
    debug("%s#%d: NÅT ANNAT process_wait(%d) RETURNS %d\n",
          cur->name, cur->tid, child_id, status);
  }

  if((child != NULL) && (child->parent_id == cur->tid))
  {
    //Child is found
    lock_acquire(&child->proc_lock);
    while(child->alive == true)
    {
      cond_wait(&child->proc_cond, &child->proc_lock);
    }
    status = child->exit_code;
    // debug("%s#%d: FELIX SIN LINJA process_wait(%d) RETURNS %d\n",
    // cur->name, cur->tid, child_id, status);
    lock_release(&child->proc_lock);
  }

  debug("%s#%d: process_wait(%d) RETURNS %d\n",
        cur->name, cur->tid, child_id, status);

  return status;
}

/* Free the current process's resources. This function is called
   automatically from thread_exit() to make sure cleanup of any
   process resources is always done. That is correct behaviour. But
   know that thread_exit() is called at many places inside the kernel,
   mostly in case of some unrecoverable error in a thread.

   In such case it may happen that some data is not yet available, or
   initialized. You must make sure that nay data needed IS available
   or initialized to something sane, or else that any such situation
   is detected.
*/

void
process_cleanup (void)
{
  struct thread  *cur = thread_current ();
  uint32_t       *pd  = cur->pagedir;
  int status = -1;
  struct running_process *cur_process = plist_find(&plist, cur->tid);
    // debug("# %s#%d: process_cleanup() ENTERED\n", cur->name, cur->tid);
  if(cur_process != NULL)
  {
    status = cur_process->exit_code;
    // struct running_process* parent = plist_find(&plist, cur_process->parent_id);
    cur_process->alive=false;
    //LOOK FOR CHILDREN
    for(int i=0; i<LIST_SIZE; i++)
    {
      if(plist.content[i].parent_id==cur_process->id)
      {
        plist.content[i].parent_alive = false;
        if(plist.content[i].alive == false)
        {
          plist_remove(&plist, plist.content[i].id);
        }
      }
    }
    //utanför for loop
    if(cur_process->parent_alive == false)
    {
      plist_remove(&plist, cur_process->id);
    }
    lock_acquire(&cur_process->proc_lock);
    cond_signal(&cur_process->proc_cond, &cur_process->proc_lock);
    lock_release(&cur_process->proc_lock);
  }

  struct map* map = &cur->ourmap;
  for (int i = 0; i < MAP_SIZE; i++)
  {
    if(map->content[i] != NULL)
    {
      file_close(map->content[i]);
      map_remove(map,i);
    }
  }

  /* Later tests DEPEND on this output to work correct. You will have
   * to find the actual exit status in your process list. It is
   * important to do this printf BEFORE you tell the parent process
   * that you exit.  (Since the parent may be the main() function,
   * that may sometimes poweroff as soon as process_wait() returns,
   * possibly before the printf is completed.)
   */

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  if (pd != NULL)
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
  debug("%s#%d: process_cleanup() DONE with status %d\n",
        cur->name, cur->tid, status);
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}
