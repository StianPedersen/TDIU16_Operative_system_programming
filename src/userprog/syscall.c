#include <stdio.h>
#include <syscall-nr.h>
#include "userprog/syscall.h"
#include "threads/interrupt.h"
#include "threads/thread.h"

/* header files you probably need, they are not used yet */
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "flist.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* This array defined the number of arguments each syscall expects.
   For example, if you want to find out the number of arguments for
   the read system call you shall write:

   int sys_read_arg_count = argc[ SYS_READ ];

   All system calls have a name such as SYS_READ defined as an enum
   type, see `lib/syscall-nr.h'. Use them instead of numbers.
 */
const int argc[] = {
  /* basic calls */
  // int h = argc{SYS_HALT};
  0, 1, 1, 1, 2, 1, 1, 1, 3, 3, 2, 1, 1,
  /* not implemented */
  2, 1,    1, 1, 2, 1, 1,
  /* extended */
  0
};

static void
syscall_handler (struct intr_frame *f)
{
  int32_t* esp = (int32_t*)f->esp;
  // uint32_t eax = f->eax;

  switch (esp[0] /* retrive syscall number */ )
  {
    case SYS_HALT:
      {
        // printf ("ENTERED SYS HALT!\n");
        power_off();
      }

    case SYS_EXIT:
      {
        // printf ("ENTERED SYS EXIT!\n");
        process_exit(esp[1]);
      }

    case SYS_READ:
      {
        int fd= esp[1];
        char* buffer = esp[2];
        unsigned int length = esp[3];
        char c;

        if(fd != STDOUT_FILENO)
        {
          for (size_t i = 0; i < length; i++)
          {
            c = input_getc();
            if(c=='\r')
              {
                c = '\n';
              }
            buffer[i] = c;
            printf("%c" , buffer[i]);
          }
          f->eax = length;
          return length;
        }
         f->eax = -1;
          return -1;
      }

    case SYS_CREATE:
      {
        const char* file_name= esp[1];
        unsigned int initial_size = esp[2];
        printf ("ENTERED SYS CREATE!\n");
        printf ("%s %s %d!\n", "VARIABLES:", file_name, initial_size);
        f->eax = filesys_create(file_name, initial_size);
        return f->eax;
      }
    case SYS_OPEN:
      {
        const char* file_name= esp[1];
        printf ("ENTERED SYS OPEN!\n");
        f->eax = filesys_open(file_name);
        printf ("%s %s\n", "FILE_NAME:", file_name);
        if(f->eax == NULL)
        {
          f->eax = -1;
          return f->eax;
        }
        int i = map_insert(f->eax, file_name);
        printf ("%s %d!\n", "map_insert:", i);
        return i;
      }

    case SYS_WRITE:
      {
        int fd= esp[1];
        char* buffer = esp[2];
        unsigned int length = esp[3];

        if(fd != STDIN_FILENO)
        {
          putbuf(buffer, length);
          f->eax = length;
          return length;
        }
        f->eax = -1;
          return -1;

      }
    default:
    {
      printf ("Executed an unknown system call!\n");

      printf ("Stack top + 0: %d\n", esp[0]);
      printf ("Stack top + 1: %d\n", esp[1]);

      thread_exit ();
    }
  }
}
