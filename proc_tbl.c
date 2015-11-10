#include "proc_tbl.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define CMDLINE_NAME "/proc/%d/cmdline"
#define EXE_NAME "/proc/%d/comm"
#define MAX_PROCFILE_LENGTH 128

bool proc_fill(pid_t pid, time_t start_time, proc_t *proc)
{
    proc->pid = pid;
    proc->start_time = start_time;

    char proc_file[MAX_PROCFILE_LENGTH];
    // Read exe
    snprintf(proc_file, MAX_PROCFILE_LENGTH, EXE_NAME, pid);
    FILE *f = fopen(proc_file, "r");
    size_t size = 0;
    if (f) {
        size = fread(proc->exe, sizeof(char), COMM_MAX, f);
        if (size > 0) {
            if (proc->exe[size-1] != '\0') {
                proc->exe[size-1] = '\0';
            }
        }
        fclose(f);
    }
    return (size > 0);
}

proc_t *get_proc(proc_t *proc, pid_t pid)
{
    proc_t *runner = proc;
    while (runner != NULL)
    {
        if (runner->pid == pid)
        {
            return runner;
        }
        runner = runner->next;
    }
    return NULL;
}

proc_t *insert_proc(proc_t *bucket, proc_t *p)
{
    if (bucket == NULL) {
        return p;
    } else {
        proc_t *proc = get_proc(bucket, p->pid);
        if (proc == NULL) {
            proc_t *runner = bucket;
            while (runner->next != NULL) {
                runner = runner->next;
            }
            runner->next = p;
        } else {
            memcpy(proc->exe, p->exe, COMM_MAX);
            proc->start_time = p->start_time;
            free(p);
        }
    }
    return bucket;
}

proc_t *remove_proc(proc_t *bucket, pid_t pid)
{
    if (bucket->pid == pid)
    {
        free(bucket);
        return NULL;
    }
    proc_t *runner = bucket;
    proc_t *prev = NULL;
    while (runner != NULL)
    {
        if (runner->pid == pid && prev != NULL)
        {
            prev->next = runner->next;
            free(runner);
            return bucket;
        }
        prev = runner;
        runner = runner->next;
    }

    return NULL;
}
