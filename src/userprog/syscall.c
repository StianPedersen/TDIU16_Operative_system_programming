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
#include "devices/timer.h"

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
  if(!verify_fix_length(f->esp, sizeof(esp)))
  {
    f->eax = -1;
    process_exit(f->eax);
  }
  if(esp[0] < 0 || esp[0] > 21)
  {
    f->eax = -1;
    process_exit(f->eax);
  }

  int sys_read_arg_count = argc[esp[0]];
  if(!verify_fix_length(esp+1, (sizeof(esp)*sys_read_arg_count)))
  {
    f->eax = -1;
    process_exit(f->eax);
  }
  void*tmp = pagedir_get_page(thread_current()->pagedir, f->esp);
  //Tmp is a virtual adress, first check if its NULL
  if(tmp==NULL)
  {
    f->eax = -1;
    process_exit(f->eax);
  }


  switch (esp[0] /* retrive syscall number */ )
  {
    case SYS_HALT:
      {
        power_off();
        break;
      }

    case SYS_EXIT:
      {
        if(is_kernel_vaddr((void*)esp[1]))
        {

          f->eax = -1;
          process_exit(f->eax);
          break;
        }
        process_exit(esp[1]);
        break;
      }

    case SYS_READ:
      {
        //los kontrollos
        if(is_kernel_vaddr((void*)esp[2]))
        {
          f->eax = -1;
          process_exit(f->eax);
          break;
        }
        if(!verify_fix_length((void*)esp[2], esp[3]))
        {
          f->eax = -1;
          process_exit(f->eax);
          break;
        }

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
        if(!verify_fix_length((void*)esp[1], esp[2]))
        {
          f->eax = -1;
          process_exit(f->eax);
          break;
        }
        const char* file_name= (char*)esp[1];
        unsigned int initial_size = esp[2];
        f->eax = filesys_create(file_name, initial_size);
        break;
      }

    case SYS_OPEN:
      {
        void*tmp = pagedir_get_page(thread_current()->pagedir, (void*)esp[1]);
        if(tmp==NULL)
        {
          f->eax = -1;
          process_exit(f->eax);
          break;
        }

        const char* file_name= (char*)esp[1];
        struct file* openfile = filesys_open(file_name);
        if(openfile == NULL)
        {
          f->eax = -1;
          break;
        }

          f->eax = map_insert(&thread_current()->ourmap, openfile);
          int status = f->eax;
          if(status == -1)
          {
            filesys_close(openfile);
          }

        break;
      }

    case SYS_WRITE:
      {
        //uno kontrollo
        if(!verify_fix_length((void*)esp[2], esp[3]))
        {
          f->eax = -1;
          process_exit(f->eax);
          break;
        }
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
        int length = esp[2];
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
      process_print_list();
      break;
    }

    case SYS_SLEEP:
    {
      uint16_t init= esp[1];
      timer_init(init);
      timer_msleep(init);
      break;
    }

    case SYS_EXEC:
    {

      void*tmp = pagedir_get_page(thread_current()->pagedir, (void*)esp[1]);
      if(tmp==NULL)
      {
        f->eax = -1;
        process_exit(f->eax);
        break;
      }
      char * processname = (char*)esp[1];
      f->eax = process_execute(processname);
      break;
    }

    case SYS_WAIT:
    {
      f->eax = process_wait(esp[1]);
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

/* Verify all addresses from and including 'start' up to but excluding
 * (start+length). */
bool verify_fix_length(void* start, int length)
{
  //char* adr = pg_round_down(start);
  if(is_kernel_vaddr(start))
  {
    return false;
  }
  void* end = (void*)((char*)start + length);
  void* tmp = start;
  if(pagedir_get_page(thread_current()->pagedir, start)== NULL)
  {
    return false;
  }
  else
  {
    for(char* adr = pg_round_down(start); adr != end; adr++)
    {
      if(pg_no(adr)!=pg_no(tmp))
      {
        if(pagedir_get_page(thread_current()->pagedir, adr)== NULL)
        {
          return false;
        }
        if(is_kernel_vaddr(adr))
        {
          return false;
        }
        else
        {
          tmp=adr;
        }
      }
    }
  }
  return true;
  // ADD YOUR CODE HERE
}

/* Verify all addresses from and including 'start' up to and including
 * the address first containg a null-character ('\0'). (The way
 * C-strings are stored.)
 */
bool verify_variable_length(char* start)
{
  // ADD YOUR CODE HERE
  char* adr = start;
  if(pagedir_get_page(thread_current()->pagedir, start)== NULL)
  {
    return false;
  }
  else
  {
    // while(!is_end_of_string(start))
    // {
    while(*start != '\0')
    {
      start++;
      if(pg_no((void*)start)!=pg_no((void*)adr))
      {
        if(is_kernel_vaddr(start))
        {
          return false;
        }
        if(pagedir_get_page(thread_current()->pagedir, (void*)start)== NULL)
        {
          return false;
        }
        else
        {
          adr=start;
        }
    }
  }
  return true;
}
}
