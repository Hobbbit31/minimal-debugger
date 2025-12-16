#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/user.h> 
#include "debug.h"
#include "breakpoint.h"



unsigned long logicalrip(unsigned long real_rip){
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (!breakpoints[i].used)
            continue;

        //INT3 advances RIP by one byte, so RIP == breakpoint address + 1
        if (real_rip == breakpoints[i].addr + 1) {
            return breakpoints[i].addr;
        }

        //RIP moved past INT3; map nearby instruction back to breakpoint for display
        if (real_rip > breakpoints[i].addr && real_rip - breakpoints[i].addr <= 16) {
            return breakpoints[i].addr;
        }
    }

    return real_rip;
}


/*
 * Step one CPU instruction.
 * This means: do only ONE tiny move and stop again.
 */
int debuggerStep(Debugger *dbg){

    //Cannot step if program has already exited
    if (dbg->state == EXITED) {
        printf("[dbg](step,debugger.c) cannot step: program has exited\n");
        return -1;
    }

    // Single-step is only allowed when the program is stopped
    if (dbg->state != STOPPED) {
        printf("[dbg](step,debugger.c) cannot step: program is not stopped\n");
        return -1;
    }

    // Ask kernel to execute exactly one instruction
    if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace SINGLESTEP(step,debugger.c)");
        return -1;
    }

    // Wait for the child to stop or exit
    int status;
    if (waitpid(dbg->child_pid, &status, 0) == -1) {
        perror("waitpid(step,debugger.c)");
        return -1;
    }

    // If the program exited during single-step
    if (WIFEXITED(status)) {
        dbg->state = EXITED;
        printf("[dbg](step,debugger.c) child exited\n");
        return 0;
    }


    // Otherwise, the program stopped again
    dbg->state = STOPPED;
    return 0;
}


void printRegisters(Debugger *dbg , state st){
    struct user_regs_struct r;
    if(ptrace(PTRACE_GETREGS, dbg->child_pid, NULL, &r) == -1){
        perror("Error getregs(debugger.c)");
        return;
    }
    
    if(st == STACK || st == ALL){
        unsigned long lrip = logicalrip(r.rip);;
        printf("RIP: 0x%lx\n", lrip);
        printf("RSP: 0x%llx\n", r.rsp);
        printf("RBP: 0x%llx\n", r.rbp);
    }

    if(st == GENERAL || st == ALL){
        printf("RAX: 0x%llx\n", r.rax);
        printf("RBX: 0x%llx\n", r.rbx);
        printf("RCX: 0x%llx\n", r.rcx);
        printf("RDX: 0x%llx\n", r.rdx);
        printf("RSI: 0x%llx\n", r.rsi);
        printf("RDI: 0x%llx\n", r.rdi);

        printf("R8 : 0x%llx\n", r.r8);
        printf("R9 : 0x%llx\n", r.r9);
        printf("R10: 0x%llx\n", r.r10);
        printf("R11: 0x%llx\n", r.r11);
        printf("R12: 0x%llx\n", r.r12);
        printf("R13: 0x%llx\n", r.r13);
        printf("R14: 0x%llx\n", r.r14);
        printf("R15: 0x%llx\n", r.r15);
    }
    

}

/*
 * This function fixes breakpoints.
 * It does the magic:
 * 1) remove INT3
 * 2) fix RIP
 * 3) step once
 * 4) put INT3 back
 * 5) 1- breakpoint handled  0 - not a breakpoint -1 - error
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
        perror("ptrace SETREGS(handleBP,debugger.c)");
        return -1;
    }

    // Execute the original instruction once
    if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace SINGLESTEP (bp) (handleBP,debugger.c)");
        return -1;
    }

    int status;
    waitpid(dbg->child_pid, &status, 0);
    if(WIFEXITED(status)){
        dbg->state = EXITED;
    }

    // put breakpoint back 
    setBP(dbg->child_pid, bp_addr);

    printf("[dbg](handleBP,debugger.c) breakpoint hit at 0x%lx\n", bp_addr);
    return 1;
}


/*
 * Start the program we want to debug.
 * Parent = debugger
 * Child = program
 */
int launchDebugger(Debugger *dbg, char *prog, char **args){
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        // Child: request tracing by parent 
        ptrace(PTRACE_TRACEME, 0, 0, 0);

        // Replace process image with target program
        execvp(prog, args);
        perror("execvp (launchBP,debugger.c)");
        _exit(1);
    }

    // Parent waits for child to stop after exec
    waitpid(pid, &status, 0);

    dbg->lastStatus = status;

    dbg->child_pid = pid;
    dbg->state = STOPPED;

    printf("[dbg](launchBP,debugger.c) child %d stopped (ready)\n", pid);
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
        printf("[dbg](continueDBG,debugger.c) program has already exited\n");
        return 0;
    }

    dbg->state = RUNNING;
    // Resume executio
    if (ptrace(PTRACE_CONT, dbg->child_pid, 0, 0) == -1) {
        perror("ptrace CONT (continueDBG,debugger.c) ");
        dbg->state = STOPPED;
        return -1;
    }

    // Wait for the next event
    if (waitpid(dbg->child_pid, &status, 0) == -1) {
        perror("waitpid (continueDBG,debugger.c) ");
        dbg->state =STOPPED;
        return -1;
    }
    dbg->lastStatus = status;

    // Program exited normally
    if (WIFEXITED(status)) {
        printf("[dbg](continueDBG,debugger.c)  child exited with status %d\n", WEXITSTATUS(status));
        dbg->state = EXITED;
        return 0;
    }

    // Program stopped due to SIGTRAP (breakpoint or single-step)
    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
        // if (handleBP(dbg)) {
        //     dbg->state = STOPPED;
        //     return 0;   // breakpoint stop
        // }
        // Handle breakpoint if this SIGTRAP is due to INT3
        int bp = handleBP(dbg);
        if (bp < 0) {
            return -1;
        }
        if (bp == 1) {
        // real breakpoint was hit and fixed
            dbg->state = STOPPED;
            return 0;
        }

        // SIGTRAP but NOT a breakpoint (likely single-step)
        dbg->state = STOPPED;
        return 0;
    }

    return 0;
}


void printProcessStatus(Debugger *dbg){

    if (dbg->state == NOT_STARTED) {
        printf("Process state: NOT_STARTED\n");
        return;
    }

    if (dbg->state == RUNNING) {
        printf("Process state: RUNNING\n");
        return;
    }

    if (dbg->state == EXITED) {
        printf("Process state: EXITED\n");
        return;
    }

    /* STOPPED → need kernel reason */
    int status = dbg->lastStatus;

    if (WIFSTOPPED(status)) {
        printf("Process state: STOPPED (signal %d)\n",
               WSTOPSIG(status));
    }
    else {
        printf("Process state: STOPPED\n");
    }
}


int stopDebugger(Debugger *dbg){
    if (dbg->state != RUNNING)
        return 0;

    // Send SIGSTOP to pause the child
    kill(dbg->child_pid, SIGSTOP);

    int status;
    waitpid(dbg->child_pid, &status, 0);

    dbg->lastStatus = status;
    dbg->state = STOPPED;
    return 0;
}