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

#include "plist.h"

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

  switch (esp[0] /* retrive syscall number */ )
  {
    case SYS_HALT:
      {
        power_off();
        break;
      }

    case SYS_EXIT:
      {
        process_exit(esp[1]);
        break;
      }

    case SYS_READ:
      {
        int fd= esp[1];
        char* buffer = (char*)esp[2];
        unsigned int length = esp[3];
        char c;

        if(fd == STDIN_FILENO)
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
          break;
        }
        else if(map_find(&thread_current()->ourmap,fd) != NULL)
        {
          struct file* filen = map_find(&thread_current()->ourmap,fd);
          f->eax= file_read(filen, buffer, length);
          break;
        }
         f->eax = -1;
         break;
      }

    case SYS_CREATE:
      {
        const char* file_name= (char*)esp[1];
        unsigned int initial_size = esp[2];
        printf ("%s %s %d!\n", "VARIABLES:", file_name, initial_size);
        f->eax = filesys_create(file_name, initial_size);
        break;
      }

    case SYS_OPEN:
      {
        const char* file_name= (char*)esp[1];
        struct file* openfile = filesys_open(file_name);
        printf ("%s %s\n", "FILE_NAME:", file_name);
        if(openfile == NULL)
        {
          f->eax = -1;
          break;
        }

        f->eax = map_insert(&thread_current()->ourmap, openfile);
        if(f->eax == -1)
        {
          filesys_close(openfile);
        }
        break;
      }

    case SYS_WRITE:
      {
        int fd= esp[1];
        char* buffer = (char*)esp[2];
        unsigned int length = esp[3];
        struct file* filen = map_find(&thread_current()->ourmap,fd);
        if(fd == STDOUT_FILENO)
        {
          putbuf(buffer, length);
          f->eax = length;
          break;
        }
        else if(filen != NULL)
        {
          file_write(filen,buffer,length);
          f->eax = length;
          break;
        }
        f->eax = -1;
        break;
      }

    case SYS_CLOSE:
      {
        int fd= esp[1];
        struct file *file = map_find(&thread_current()->ourmap,fd);
        if(file != NULL)
        {
          filesys_close(file);
          map_remove(&thread_current()->ourmap,fd);
        }
        break;
      }

    case SYS_REMOVE:
      {
        char* name = (char*)esp[1];
        f->eax = filesys_remove(name);
        break;
      }

    case SYS_SEEK:
      {
        int fd= esp[1];
        unsigned int length = esp[2];
        struct file* filen = map_find(&thread_current()->ourmap,fd);
        if (filen != NULL && length < file_length(filen))
        {
          file_seek(filen,length);
        }
        break;
      }

    case SYS_TELL:
      {
        int fd= esp[1];
        struct file* filen = map_find(&thread_current()->ourmap,fd);
        if (filen != NULL)
        {
          f->eax = file_tell(filen);
          break;
        }
        f->eax = -1;
        break;
      }

    case SYS_FILESIZE:
      {
        int fd= esp[1];
        struct file* filen = map_find(&thread_current()->ourmap,fd);
        if (filen != NULL)
        {
          f->eax = file_length(filen);
          break;
        }
        f->eax = -1;
        break;
      }

    case SYS_PLIST:
    {
      printf("KOMMER TILL SYSCALL SYS_PLIST\n\n");
      process_print_list();
      break;
    }

    case SYS_EXEC:
    {



      break;
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
