#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/user.h> 
#include "debug.h"
#include "breakpoint.h"


/*
 * Step one CPU instruction.
 * This means: do only ONE tiny move and stop again.
 */
int debuggerStep(Debugger *dbg){

    //Cannot step if program has already exited
    if (dbg->state == EXITED) {
        printf("[dbg] cannot step: program has exited\n");
        return -1;
    }

    // Single-step is only allowed when the program is stopped
    if (dbg->state != STOPPED) {
        printf("[dbg] cannot step: program is not stopped\n");
        return -1;
    }

    // Ask kernel to execute exactly one instruction
    if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace SINGLESTEP");
        return -1;
    }

    // Wait for the child to stop or exit
    int status;
    if (waitpid(dbg->child_pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    // If the program exited during single-step
    if (WIFEXITED(status)) {
        dbg->state = EXITED;
        printf("[dbg] child exited\n");
        return 0;
    }


    // Otherwise, the program stopped again
    dbg->state = STOPPED;
    return 0;
}


void printRegisters(Debugger *dbg){
    struct user_regs_struct r;
    ptrace(PTRACE_GETREGS, dbg->child_pid, NULL, &r);

    printf("RIP: 0x%llx\n", r.rip);
    printf("RSP: 0x%llx\n", r.rsp);
    printf("RBP: 0x%llx\n", r.rbp);
    printf("RAX: 0x%llx\n", r.rax);
}

/*
 * This function fixes breakpoints.
 * It does the magic:
 * 1) remove INT3
 * 2) fix RIP
 * 3) step once
 * 4) put INT3 back
 */
int handleBP(Debugger *dbg){
    struct user_regs_struct regs;

    // Read current register state
    ptrace(PTRACE_GETREGS, dbg->child_pid, 0, &regs);

    unsigned long rip = regs.rip;
    unsigned long bp_addr = 0;
    int found = 0;

    //  find which breakpoint was hit 
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (breakpoints[i].used &&
            rip == breakpoints[i].addr + 1) {
            bp_addr = breakpoints[i].addr;
            found = 1;
            break;
        }
    }

    // If SIGTRAP was not caused by our breakpoint
    if (!found)
        return 0;

    // Restore original instruction (remove INT3)
    clearBP(dbg->child_pid, bp_addr);

    // Move RIP back to the real instruction
    regs.rip = bp_addr;
    if (ptrace(PTRACE_SETREGS, dbg->child_pid, 0, &regs) == -1) {
        perror("ptrace SETREGS");
        return -1;
    }

    // Execute the original instruction once
    if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace SINGLESTEP (bp)");
        return -1;
    }
    waitpid(dbg->child_pid, NULL, 0);

    // put breakpoint back 
    setBP(dbg->child_pid, bp_addr);

    printf("[dbg] breakpoint hit at 0x%lx\n", bp_addr);
    return 1;
}


/*
 * Start the program we want to debug.
 * Parent = debugger
 * Child = program
 */
int launchDebugger(Debugger *dbg, char *prog, char **args){
    pid_t pid = fork();

    if (pid == 0) {
        // Child: request tracing by parent 
        ptrace(PTRACE_TRACEME, 0, 0, 0);

        // Replace process image with target program
        execvp(prog, args);
        perror("execvp");
        _exit(1);
    }

    // Parent waits for child to stop after exec
    waitpid(pid, NULL, 0);

    dbg->child_pid = pid;
    dbg->state = STOPPED;

    printf("[dbg] child %d stopped (ready)\n", pid);
    return 0;
}




/*
 * Continue running the program.
 * Stop when:
 * - breakpoint
 * - program ends
 */
int continueDebugger(Debugger *dbg){
    int status;

    // Cannot continue an exited program
    if (dbg->state == EXITED) {
        printf("[dbg] program has already exited\n");
        return 0;
    }

    // Resume executio
    if (ptrace(PTRACE_CONT, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace CONT");
        return -1;
    }

    // Wait for the next event
    if (waitpid(dbg->child_pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    // Program exited normally
    if (WIFEXITED(status)) {
        printf("[dbg] child exited with status %d\n", WEXITSTATUS(status));
        dbg->state = EXITED;
        return 0;
    }

    // Program stopped due to SIGTRAP (breakpoint or single-step)
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        handleBP(dbg);
        dbg->state = STOPPED;
        return 0;
    }

    return 0;
}

