#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>

void syscall_init (void);

void if_user_add(void *addr);
void pop_arguments(void *esp, int *arg, int count);
void syscall_exit(int status);
bool syscall_create(const char *file , unsigned size);
bool syscall_remove(const char *file);
int syscall_exec(const *cmd_line);
int syscall_wait(int child_tid);

#endif /* userprog/syscall.h */
