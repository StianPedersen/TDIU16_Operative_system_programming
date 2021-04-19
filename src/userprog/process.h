#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

int count_args(const char* buf, const char* delimeters);
bool exists_in(char c, const char* d);
void* setup_main_stack(const char* command_line, void* stack_top);
void dump(void* ptr, int size);

void process_init (void);
void process_print_list (void);
void process_exit (int status);
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_cleanup (void);
void process_activate (void);

/* This is unacceptable solutions. */
#define INFINITE_WAIT() for ( ; ; ) thread_yield()
#define BUSY_WAIT(n)       \
    do {                   \
      int i = n;           \
      while ( i --> 0 )    \
        thread_yield();    \
    } while ( 0 )

#endif /* userprog/process.h */
