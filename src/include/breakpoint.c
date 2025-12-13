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

// adding breakpoint address to atable
int addBP(unsigned long addr)
{
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

    long word = ptrace(PTRACE_PEEKDATA, pid, add, 0);
    breakpoints[idx].orig_word = word;

    long int3 = (word & ~0xFF) | 0xCC;
    ptrace(PTRACE_POKEDATA, pid, add, int3);

    // errno = 0;
    // long word = ptrace(PTRACE_PEEKDATA, pid, (void *)addr, NULL);
    // if (word == -1 && errno) {
    //     perror("ptrace PEEKDATA");
    //     return -1;
    // }

    // /* save original instruction word */
    // breakpoints[idx].orig_word = word;

    // /* replace lowest byte with INT3 (0xCC) */
    // long new_word = (word & ~0xFF) | 0xCC;

    // if (ptrace(PTRACE_POKEDATA, pid, (void *)addr, (void *)new_word) == -1) {
    //     perror("ptrace POKEDATA");
    //     return -1;
    // }

    // // printf("[dbg] breakpoint set at 0x%lx\n", addr);
    // return 0;
}

int clearBP(pid_t pid , unsigned long addr){
    int idx = findBP(addr);
    if (idx < 0) {
        fprintf(stderr, "[dbg] breakpoint not found\n");
        return -1;
    }

    unsigned long add = breakpoints[idx].addr;
    long word = breakpoints[idx].orig_word;

    ptrace(PTRACE_POKEDATA, pid, add, word);

    // errno = 0;
    // long word = ptrace(PTRACE_PEEKDATA, pid, (void *)addr, NULL);
    // if (word == -1 && errno) {
    //     perror("ptrace PEEKDATA");
    //     return -1;
    // }

    // /* restore lowest byte from saved instruction */
    // long restored =
    //     (word & ~0xFF) | (breakpoints[idx].orig_word & 0xFF);

    // if (ptrace(PTRACE_POKEDATA, pid, (void *)addr, (void *)restored) == -1) {
    //     perror("ptrace POKEDATA");
    //     return -1;
    // }

    // printf("[dbg] breakpoint removed at 0x%lx\n", addr);
    return 0;
}

