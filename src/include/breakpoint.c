#include "breakpoint.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>

Breakpoint breakpoints[32];


void intializationBP(void)
{
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        breakpoints[i].used = 0;
    }
}

// adding breakpoint address to table
int addBP(unsigned long addr)
{
    if (addr == 0 || (addr & 0x1)) {
        fprintf(stderr, "[dbg] invalid breakpoint address\n");
        return -1;
    }

    if (findBP(addr) >= 0) {
        fprintf(stderr, "[dbg] breakpoint already exists at 0x%lx\n", addr);
        return -1;
    }

    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (!breakpoints[i].used) {
            breakpoints[i].addr = addr;
            breakpoints[i].used = 1;
            breakpoints[i].orig_word = 0;
            return 0;
        }
    }
    fprintf(stderr, "[dbg] breakpoint table full\n");
    return -1;  // table full
}

// find BP from table
int findBP(unsigned long addr)
{
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (breakpoints[i].used && breakpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

// remove BP
int removeBP(unsigned long addr)
{
    int idx = findBP(addr);
    if (idx == -1)
        return -1;

    breakpoints[idx].used = 0;
    breakpoints[idx].orig_word = 0;
    return 0;
}

// inserting int3(0xcc) at the specified address
int setBP(pid_t pid , unsigned long addr){ 
    int idx = findBP(addr);
    if (idx < 0) {
        fprintf(stderr, "[dbg] breakpoint not registered\n");
        return -1;
    }

    unsigned long add = breakpoints[idx].addr;

    errno = 0;
    long word = ptrace(PTRACE_PEEKDATA, pid, add, 0);
    if (word == -1 && errno != 0) {
        perror("[dbg] PTRACE_PEEKDATA");
        return -1;
    }

    // Save original instruction for later restoration
    breakpoints[idx].orig_word = word;

    // Replace lowest byte with INT3 (0xCC)
    long int3 = (word & ~0xFF) | 0xCC;

    if (ptrace(PTRACE_POKEDATA, pid, add, int3) == -1) {
        perror("[dbg] PTRACE_POKEDATA");
        return -1;
    }

    return 0;
}

int clearBP(pid_t pid , unsigned long addr){
    int idx = findBP(addr);
    if (idx < 0) {
        fprintf(stderr, "[dbg] breakpoint not found\n");
        return -1;
    }

    unsigned long add = breakpoints[idx].addr;
    long word = breakpoints[idx].orig_word;

    if (ptrace(PTRACE_POKEDATA, pid, add, word) == -1) {
        perror("[dbg] PTRACE_POKEDATA restore");
        return -1;
    }

    return 0;
}

