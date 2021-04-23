#include <stddef.h>
#include "plist.h"
#include <stdio.h>




  static struct lock pid_lock;


void plist_init(struct process_list* plist)
{
    lock_init(&pid_lock);
    printf("PLIST INIT\n");
    for(int i =0; i<LIST_SIZE; i++)
    {
      plist->content[i].free=true;
      sema_init(&(plist->content[i].process_sema),0);
      lock_init(&plist->content[i].proc_lock);
      cond_init(&plist->content[i].proc_cond);
    }

  };

int plist_insert(struct process_list* plist, int id, int parent_id, char* name)
{
  lock_acquire(&pid_lock);
  for (unsigned int i = 0; i < LIST_SIZE; i++)
    {
      if(plist->content[i].free == true)
        {
          plist->content[i].free =false;
          lock_release(&pid_lock);
          plist->content[i].id=id;
          plist->content[i].parent_id=parent_id;
          plist->content[i].alive=true;
          plist->content[i].name=name;
          return id;
        }
    }
    lock_release(&pid_lock);
    return -1;
}

void print_list(struct process_list* plist)
{
  //lock_acquire(&pid_lock);
  for(int i=0; i<LIST_SIZE; i++)
  {
    if(plist->content[i].free==false)
    {
      printf("PROCESS NAME: %s, PROCESS ID: %d, PARENT ID: %d \n\n", plist->content[i].name, plist->content[i].id, plist->content[i].parent_id);
    }
  }
  //lock_release(&pid_lock);
}

struct running_process* plist_find(struct process_list* plist, int id)
{
  lock_acquire(&pid_lock);
  for (unsigned int i = 0; i < LIST_SIZE; i++)
    {
      if(plist->content[i].id == id)
        {

          lock_release(&pid_lock);
          return &plist->content[i];
        }
    }
    lock_release(&pid_lock);
    return NULL;
}

void plist_remove(struct process_list* plist, int id)
{
  lock_acquire(&pid_lock);
  for (unsigned int i = 0; i < LIST_SIZE; i++)
    {
      if(plist->content[i].id == id)
        {
          plist->content[i].free = true;
          lock_release(&pid_lock);
          break;
        }
    }
}
