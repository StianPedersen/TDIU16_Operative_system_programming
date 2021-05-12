#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
// void halt (void);
// void exit(int status);
bool verify_fix_length(void*, int);
bool verify_variable_length(char*);
#endif /* userprog/syscall.h */
