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

  // printf("syscall num is %d \n", *(int *)f->esp);
  // printf("assr is %X \n", f->esp+4);

  /* project 2 */
  switch(*(int *)f->esp){
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT:
      if_user_add(f->esp+4); 
      syscall_exit(*(int *)(f->esp+4));
      break;
    case SYS_CREATE:
      
      if_user_add(f->esp+8);
      f->eax = syscall_create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      if_user_add(f->esp+4); 
      f->eax = syscall_remove((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_EXEC:
      if_user_add(f->esp+4);
      /*project 3*/
      //is_valid_str((const char *)*(uint32_t *)(f->esp+4));
      f->eax = syscall_exec((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_WAIT:
      if_user_add(f->esp+4); 
      f->eax = syscall_wait((tid_t*)*(uint32_t *)(f->esp+4)); //
      break;
    case SYS_OPEN:
      if_user_add(f->esp+4); 
      /*project 3*/
      //is_valid_str((const char *)*(uint32_t *)(f->esp+4));
      f->eax = syscall_open((const char *)*(uint32_t *)(f->esp+4));
      break;
    case SYS_FILESIZE:
      if_user_add(f->esp+4); 
      f->eax = syscall_filesize(*(int*)(f->esp+4));
      break;
    case SYS_READ:
      if_user_add(f->esp+12); 
      /*project 3*/
      is_valid_buffer((const char *)*(uint32_t *)(f->esp + 8), *(unsigned int *)(f->esp + 12), *(int*)(f->esp + 4), 1);
      f->eax = syscall_read(*(int*)(f->esp + 4), (const char *)*(uint32_t *)(f->esp + 8), *(unsigned int *)(f->esp + 12));
      break;
    case SYS_WRITE:
      if_user_add(f->esp+12); 
      /*project 3*/
      is_valid_buffer((const char *)*(uint32_t *)(f->esp + 8), *(unsigned int *)(f->esp + 12), *(int*)(f->esp + 4), 0);
      f->eax = syscall_write((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8), *(unsigned *)(f->esp + 12));
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
    case SYS_MMAP:
      if_user_add(f->esp+8); 
      f->eax = syscall_mmap((int)*(uint32_t *)(f->esp + 4), (void *)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_MUNMAP:
      if_user_add(f->esp+4); 
      syscall_munmap((int)*(uint32_t *)(f->esp + 4));
      break;
  }
  /* projext 2 1107*/
  // thread_exit ();
}

// void if_user_add(void *addr)
// {
//   /* project 2  */
//   if(is_user_vaddr(addr)){
//     return;
//   }
//   syscall_exit(-1);
// }

/*project 3*/
struct sp_entry *if_user_add(void *addr)
{
  if(addr < (void *)0x08048000 || addr >= (void *)0xc0000000){
    syscall_exit(-1);
    // return NULL; // add 
  }

  struct sp_entry *spe = find_spe(addr);
  if(spe==NULL) syscall_exit(-1);
  //printf("is loaded %d \n", spe->is_loaded);

  return spe;
}

void syscall_exit(int status)
{
  struct thread* cur = thread_current();
  cur->exit_status = status;
  
  /*projext2 */
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit();
}

bool syscall_create(const char *file , unsigned int size)
{
  if_user_add(file);
  if (*file==NULL) // create-null test
  {
    //ASSERT(0);
    syscall_exit(-1);
  }

  return filesys_create(file, size);
}

bool syscall_remove(const char *file)
{
  return filesys_remove(file);
}

int syscall_exec(const *cmd_line)
{
  if_user_add(cmd_line);

  tid_t tmp = process_execute(cmd_line); 
  if (tmp==TID_ERROR) // exec missing test case 
  {
    return -1;
  }

  struct thread* t = search_pid(tmp); // t= 자식 프로세스 
  
  // project 2 1108
  sema_down(&(t->wait_load));
  if(t->is_load){
    return tmp;
  }
  return -1;
}

int syscall_wait(int child_tid)
{
  return process_wait(child_tid);
}

int syscall_open(const char *file)
{
  if_user_add(file);

  if (file==NULL)
  {
    return -1;
  }
  struct file* tmp = filesys_open(file); 
  if(tmp==NULL){ 
    return -1;
  }
  //project 2 
  if(strcmp(file, thread_current()->name)==0)
  {
    file_deny_write(tmp);
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

  if (fd <2 || fd > thread_current()->fdMax)
  { return ; }

    
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
  if (fd <2 || fd > thread_current()->fdMax)
  { return ; }

  process_remove_fdTable(fd);
}

void
is_valid_buffer(void* buffer, unsigned size, void *esp, bool writable)
{
  if_user_add(buffer);
  if_user_add(buffer + size);

  for(int i = 0; i<size; i++) 
  {
    struct sp_entry *temp = find_spe(buffer+i);
    if(writable && (temp->writable != true)) syscall_exit(-1); // if buffer에 write 수행
  }
}

void
is_valid_str(const char *str)
{
  for(int i=0;i<strlen(str);i++){
    if_user_add(str+i);
  }
}

/*project 3*/
int
syscall_mmap(int fd, void* addr)
{
  if(fd < 2) return -1;
  if(((int)addr%PGSIZE)!=0) return -1;

  struct file* file;
  file = process_search_fdTable(fd);
  if(file_length(file)==0) return -1;
  struct file* reopen = file_reopen(file);

  struct thread* t=thread_current();
  
  struct mmap_file* mfile = malloc(sizeof(struct mmap_file));
  list_init(&(mfile->spe_list));
  // Alloc mapid
  mfile->mapid = t->mapid;
  t->mapid++;
  // set file
  mfile->file = reopen;
  
  list_push_back(&(t->mmap_file_list), &(mfile->elem));

  int f_size = syscall_filesize(fd);
  void* _addr = addr;
  while(f_size>0){
    struct sp_entry* spe = malloc(sizeof(struct sp_entry));
    spe->type = VM_FILE;
    spe->vaddr = _addr;
    spe->writable = true;
    spe->is_loaded = false;
    spe->file = reopen;
    spe->offset = (size_t)_addr-(size_t)addr;
    spe->read_bytes = PGSIZE;
    spe->zero_bytes = 0;

    if(f_size<PGSIZE) {
      spe->read_bytes=f_size;
      spe->zero_bytes = PGSIZE - f_size;
    }

    if(!insert_spe(&(t->sp_table), &(spe->elem))) return -1;
    
    list_push_back(&(mfile->spe_list), &(spe->mmap_elem));

    _addr += PGSIZE;
    f_size -= spe->read_bytes;
  }

  return mfile->mapid;
}

void 
syscall_munmap(int mapping)
{
  struct thread* t = thread_current();
  struct list_elem *curr1;
  struct list_elem *curr2;
  struct list_elem *curr3;
  // delete every spe
  for(curr1=list_begin(&(t->mmap_file_list));curr1!=list_end(&(t->mmap_file_list));curr1=list_next(curr1))
  {
    struct mmap_file * temp = list_entry(curr1, struct mmap_file, elem);
    if(temp->mapid == mapping || mapping == -1){
      //delete spe
      for(curr2=list_begin(&(temp->spe_list));curr2!=list_end(&(temp->spe_list));curr2=list_next(curr2))
      {
        struct sp_entry* spe = list_entry(curr2, struct sp_entry, mmap_elem);
        //write-back
        if(pagedir_is_dirty(t->pagedir, spe->vaddr)==1){
          file_write_at(spe->file, spe->vaddr, spe->read_bytes, spe->offset);
        }
        
        //evict from frame table
        for(curr3=list_begin(&(frame_table));curr3!=list_end(&(frame_table));curr3=list_next(curr3)){
          if(list_entry(curr3, struct frame_table_entry, f_elem)->kadd == pagedir_get_page(t->pagedir, spe->vaddr)){
            pagedir_clear_page(t->pagedir, spe->vaddr);
            palloc_free_page(pagedir_get_page(t->pagedir, spe->vaddr));
            curr3 = list_remove(curr3);
            free(list_entry(curr3, struct frame_table_entry, f_elem));
          }
        }
        //delete spe and deallocate
        curr2 = list_remove(curr2);
        delete_spe(&(t->sp_table), spe);
        free(spe);
      }
    }
    file_close(temp->file);
    free(temp);
  }
}