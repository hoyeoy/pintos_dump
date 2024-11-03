#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
//test
#include "threads/interrupt.h"
#include "userprog/syscall.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/*project2*/
void arg_stk(char ** argv, uint32_t argc, struct intr_frame * if_);
//void passing_argument(char **arguments, int count, char **esp);

#endif /* userprog/process.h */
