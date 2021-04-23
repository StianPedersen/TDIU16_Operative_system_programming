#ifndef _PLIST_H_
#define _PLIST_H_

#include "threads/synch.h"
/* Place functions to handle a running process here (process list).

   plist.h : Your function declarations and documentation.
   plist.c : Your implementation.

   The following is strongly recommended:

   - A function that given process inforamtion (up to you to create)
     inserts this in a list of running processes and return an integer
     that can be used to find the information later on.

   - A function that given an integer (obtained from above function)
     FIND the process information in the list. Should return some
     failure code if no process matching the integer is in the list.
     Or, optionally, several functions to access any information of a
     particular process that you currently need.

   - A function that given an integer REMOVE the process information
     from the list. Should only remove the information when no process
     or thread need it anymore, but must guarantee it is always
     removed EVENTUALLY.

   - A function that print the entire content of the list in a nice,
     clean, readable format.

 */
 #define LIST_SIZE 100
 struct running_process
 {
   int id;
   int parent_id;
   int exit_code;
   bool parent_alive;
   bool free;
   bool alive;
   char* name;
   struct semaphore process_sema;
 };



struct process_list
{
    struct running_process content [LIST_SIZE];
  };

void plist_init(struct process_list* plist);

int plist_insert(struct process_list* plist, int id, int parentID, char* name);

void print_list(struct process_list* plist);

struct running_process* plist_find(struct process_list* plist, int id);

void plist_remove(struct process_list* plist, int id);

//void plist_kill(struct process_list* plist, int id);

#endif
