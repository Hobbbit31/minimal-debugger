#include "breakpoint.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>

Breakpoint breakpoints[32];


void intializationBP(void){
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoints[i].used = 0;
    }
}

// adding breakpoint address to atable
int addBP(unsigned long addr){

    if (findBP(addr) >= 0) {
        return -2;  // already exists
    }
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (!breakpoints[i].used) {
            breakpoints[i].addr = addr;
            breakpoints[i].used = 1;
            return 0;
        }
    }
    return -1;  // table full
}

// find BP from table
int findBP(unsigned long addr){
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (breakpoints[i].used && breakpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

// remove BP
int removeBP(unsigned long addr){
    int idx = findBP(addr);
    if (idx == -1)
        return -1;

    breakpoints[idx].used = 0;
    return 0;
}

// inserting int3(0xcc) at the specified address
int setBP(pid_t pid , unsigned long addr){ 
    int idx = findBP(addr);
    if (idx < 0) {
        fprintf(stderr, "[dbg](setBP,breakpoint.c) breakpoint not registered\n");
        return -1;
    }

    unsigned long add = breakpoints[idx].addr;

    // Read the original instruction word
    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, add, 0);
    if(errno){
        perror("ptrace_peekdata(setBP,breakpoint.c)");
        return -1;
    }

    // Save original instruction for later restoration
    breakpoints[idx].orig_word = word;

    // Replace lowest byte with INT3 (0xCC)
    long int3 = (word & ~0xFF) | 0xCC;

    // Write modified instruction back to the process
    ptrace(PTRACE_POKEDATA, pid, add, int3);

    return 0;
}


int clearBP(pid_t pid , unsigned long addr){
    int idx = findBP(addr);
    if (idx < 0) {
        fprintf(stderr, "[dbg](clearBP,breakpoint.c) breakpoint not found\n");
        return -1;
    }

    unsigned long add = breakpoints[idx].addr;
    long word = breakpoints[idx].orig_word;

    // Restore the original instruction
    ptrace(PTRACE_POKEDATA, pid, add, word);


    return 0;
}

