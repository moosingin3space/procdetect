#ifndef _PROC_TBL_H
#define _PROC_TBL_H

#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

#define COMM_MAX 16

typedef struct proc_s {
    pid_t pid;
    char exe[COMM_MAX];
    //char *cmdline;
    time_t start_time;

    struct proc_s *next;
} proc_t;

bool proc_fill(pid_t pid, time_t start_time, proc_t *proc);
proc_t *insert_proc(proc_t *bucket, proc_t *p);
proc_t *get_proc(proc_t *proc, pid_t pid);
proc_t *remove_proc(proc_t *bucket, pid_t pid);

#endif
