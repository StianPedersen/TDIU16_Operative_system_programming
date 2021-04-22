#include <stddef.h>
#include "plist.h"
#include <stdio.h>




  static struct lock pid_lock;
  //struct running_process* process_list [LIST_SIZE];
  // struct process_list
  // {
  //   running_process* content [LIST_SIZE];
  // };

  void plist_init(struct process_list* plist)
  {
    lock_init(&pid_lock);
    for(int i =0; i<LIST_SIZE; i++)
    {
      printf("PLIST INIT\n");
      plist->content[i].free=true;
      sema_init(&(plist->content[i].process_sema),0);
    }

  };


int plist_insert(struct process_list* plist, int id, int parent_id)
{
  lock_acquire(&pid_lock);
  printf("\n\nPLIST INSERT\n\n");
  for (unsigned int i = 0; i < LIST_SIZE; i++)
    {
      printf("PLIST INSERT2\n");
      if(plist->content[i].free == true)
        {
          printf("PLIST INSERT3\n");
          plist->content[i].id=id;
          plist->content[i].parent_id=parent_id;
          plist->content[i].alive=true;
          plist->content[i].free =false;
          lock_release(&pid_lock);
          return id;
        }
    }
    lock_release(&pid_lock);
    return -1;
}

void print_list(struct process_list* plist)
{
  for(int i=0; i<LIST_SIZE; i++)
  {
    if(plist->content[i].free==false)
    {
      printf("PROCESS ID: %d , PARENT ID: %d \n", plist->content[i].id, plist->content[i].parent_id);
    }

  }

}
