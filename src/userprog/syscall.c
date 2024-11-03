#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include <string.h>

static void syscall_handler (struct intr_frame *);
/*Project 2*/



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int argv[3];

  switch(*(int *)f->esp){
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
      pop_arguments(f->esp, argv, 1);
      syscall_exit(argv[0]);
      break;
    case SYS_CREATE:
      pop_arguments(f->esp, argv, 2);
      f->eax = syscall_create(argv[0],argv[1]);
      break;
    case SYS_REMOVE:
      pop_arguments(f->esp, argv, 1);
      f->eax = syscall_remove(argv[0]);
      break;
  }
  thread_exit ();
}

void if_user_add(void *addr)
{
  if(is_user_vaddr(addr)){
    return;
  }
  syscall_exit(-1);
}

void pop_arguments(void *esp, int *arg, int count)
{
  esp += sizeof(char *);
  char* pointer = (char *)esp + 4;
  for(int i=0;i<count;i++){
    is_user_addr(pointer);
    arg[i] = *(int *)pointer;
    pointer += 4;
  }
}

void syscall_exit(int status)
{
  struct thread* cur = thread_current();
  printf("%s: exit(%d) \n", cur->name, status);
  thread_exit();
}

bool syscall_create(const char *file , unsigned size)
{
  return filesys_create(file, size);
}

bool syscall_remove(const char *file)
{
  return filesys_remove(file);
}

