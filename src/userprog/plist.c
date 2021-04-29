#include <stddef.h>
#include "plist.h"
#include <stdio.h>




static struct lock pid_lock;
static struct lock list_full_lock;
static struct condition list_full_condition;
void plist_init(struct process_list* plist)
{
    lock_init(&pid_lock);
    lock_init(&list_full_lock);
    cond_init(&list_full_condition);
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
    // printf("JODÅ VI KOMEMR UT UR FOR LOOPEN \n
    lock_release(&pid_lock);
    // //LIST FULL!
    plist->full_list = true;
    lock_acquire(&list_full_lock);
    while(plist->full_list)
    {
      cond_wait(&list_full_condition, &list_full_lock);
    }
    plist->content[plist->index].free =false;
    plist->content[plist->index].id=id;
    plist->content[plist->index].parent_id=parent_id;
    plist->content[plist->index].alive=true;
    plist->content[plist->index].name=name;
    lock_release(&list_full_lock);
    return id;
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
      if((plist->content[i].id == id) && (plist->content[i].alive != false))
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

          lock_acquire(&list_full_lock);
          plist->full_list = false;
          plist->index = i;
          cond_signal(&list_full_condition, &list_full_lock);
          lock_release(&list_full_lock);

          lock_release(&pid_lock);
          break;
        }
    }

}
