#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include <string.h>
#include "threads/synch.h"
#include <devices/input.h>
#include "userprog/process.h"
#include "filesys/filesys.h"


static void syscall_handler (struct intr_frame *);
/*Project 2*/
struct lock f_lock;


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&f_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if_user_add(f->esp);
  int argv[3];


  switch(*(int *)f->esp){
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
      if_user_add(f->esp+4); 
      syscall_exit(*(int *)(f->esp+4));
      break;
    case SYS_CREATE:
      //////pop_arguments(f->esp, argv, 2);
      if_user_add(f->esp+8);
      f->eax = syscall_create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      //////pop_arguments(f->esp, argv, 1);
      if_user_add(f->esp+4); 
      f->eax = syscall_remove((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_EXEC:
      //////pop_arguments(f->esp, argv, 1);
      if_user_add(f->esp+4); 
      f->eax = syscall_exec((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_WAIT:
      ////pop_arguments(f->esp, argv, 1);
      if_user_add(f->esp+4); 
      f->eax = syscall_wait((tid_t*)*(uint32_t *)(f->esp+4)); //수정됨
      break;
    case SYS_OPEN:
      ////pop_arguments(f->esp, argv, 1);
      if_user_add(f->esp+4); 
      f->eax = syscall_open((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_FILESIZE:
      /////pop_arguments(f->esp, argv, 1);
      if_user_add(f->esp+4); 
      f->eax = syscall_filesize(*(int*)(f->esp+4));
      break;
    case SYS_READ:
      ////pop_arguments(f->esp, argv, 3);
      if_user_add(f->esp+12);
      f->eax = syscall_read(*(int*)(f->esp + 4), (const char *)*(uint32_t *)(f->esp + 8), *(unsigned int *)(f->esp + 12));
      break;
    case SYS_WRITE:
      //printf("Write 실행 \n");
      /////pop_arguments(f->esp, argv, 3);
      /* projext 2 1107*/
      //printf("%p",f->esp);
      //printf("is esp now \n");
      /////f->eax = syscall_write((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      if_user_add(f->esp+12); 
      f->eax = syscall_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), *(unsigned *)(f->esp + 12));

      //printf("다음 write\n");
      // f->eax = syscall_write(argv[0],argv[1],argv[2]);
      // esp에 20을 더하는 건 4byte*5 (esp로부터 return, argv, argc, ??, ??만큼 올라 가면 argv[0] 나옴)
      break;
    case SYS_SEEK:
      syscall_seek((int)*(uint32_t *)(f->esp + 4),(int)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_TELL:
      f->eax = syscall_tell((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CLOSE:
      syscall_close((const char *)*(uint32_t *)(f->esp + 4));
      break;
  }
  /* projext 2 1107*/
  //thread_exit ();
}

void if_user_add(void *addr)
{
  /* project 2 1107 */
  /* 접근할 주소가 memory와 mapping 되어 있지 않아도 잘못된 메모리 포인터라 죽여야 함 */
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
    if_user_add(pointer);
    // arg[i] = *(int *)pointer;
    arg[i] = pointer;
    pointer += 4;
  }
}

void syscall_exit(int status)
{
  struct thread* cur = thread_current();
  cur->exit_status = status;
  
  // printf("%s: exit(%d) \n", cur->name, status);
  /*projext2 1108*/
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

bool syscall_create(const char *file , unsigned int size)
{
  if(file == NULL) syscall_exit(-1);

  return filesys_create(file, size);
}

bool syscall_remove(const char *file)
{
  return filesys_remove(file);
}

int syscall_exec(const *cmd_line)
{
  int pid = process_execute(cmd_line); // 자식 프로세스 생성 
  struct thread* t = search_pid(pid); // t= 자식 프로세스 
  
  /* project 2 1108
  if (pid == -1)
  { return -1; }
  return pid; */
  // Kernel PANIC at ../../lib/kernel/list.c:361 in find_end_of_run(): assertion `a != NULL' failed.

  // projext 2 1108
  sema_down(&(t->wait_load));
  if(t->is_load){
    return pid;
  }
  return -1;
}

int syscall_wait(int child_tid)
{
  return process_wait(child_tid);
}

int syscall_open(const char *file)
{
  if(file == NULL) return -1;

  struct file* tmp = filesys_open(file);

  if(tmp == NULL){
    return -1;
  }
  return process_add_fdTable(tmp);
}

int syscall_filesize (int fd)
{
  struct file *f = process_search_fdTable(fd);
  if(f==NULL) return -1;

  return file_length(f);
}

int
syscall_read (int fd, void *buffer, unsigned size)
{
  struct file *f;
  int i;
  int output;

  if_user_add(buffer);

  lock_acquire(&f_lock);
  if(fd == 0){
    for(i=0;i<size;i++){
      *(char *)buffer = input_getc();
      buffer++;
    }
    output = size;
  }else{
    f = process_search_fdTable(fd);
    output = file_read(f, buffer, size);
  }
  lock_release(&f_lock);

  return output;
}

int
syscall_write(int fd, void *buffer, unsigned size)
{
  struct file *f;
  int output;

  lock_acquire(&f_lock);
  if(fd == 1){
    putbuf(buffer, size);
    output = size;
  }else{
    f = process_search_fdTable(fd);
    output = file_write(f, buffer, size);
  }
  lock_release(&f_lock);

  return output;
}

void
syscall_seek (int fd, off_t position)
{
  struct file *f = process_search_fdTable(fd);
  file_seek(f, position);
}

off_t syscall_tell (int fd)
{
  struct file *f = process_search_fdTable(fd);
  return file_tell(f);
}

void syscall_close (int fd)
{
  process_remove_fdTable(fd);
}
