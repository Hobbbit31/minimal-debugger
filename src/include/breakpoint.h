#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#define MAX_BREAKPOINTS 32

typedef struct {
    unsigned long addr;   // address of breakpoint
    long orig_word;       // original instruction word
    int used;             // 0 = free, 1 = active
} Breakpoint;


extern Breakpoint breakpoints[MAX_BREAKPOINTS];


void intializationBP(void);
int  addBP(unsigned long addr);
int  findBP(unsigned long addr);
int  removeBP(unsigned long addr);
int setBP(pid_t pid , unsigned long addr);
int clearBP(pid_t pid , unsigned long addr);


#endif
