#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/user.h> 
#include "debug.h"
#include "breakpoint.h"

int debuggerStep(Debugger *dbg){
    if (dbg->state != STOPPED) {
        fprintf(stderr, "[dbg] cannot step: process not stopped\n");
        return -1;
    }

    if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, NULL, NULL) == -1) {
        perror("ptrace SINGLESTEP");
        return -1;
    }

    dbg->state = RUNNING;

    int status;
    if (waitpid(dbg->child_pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFSTOPPED(status)) {
        dbg->state = STOPPED;
        printf("[dbg] single-step stop (signal %d)\n", WSTOPSIG(status));
        return 0;
    }

    if (WIFEXITED(status)) {
        dbg->state = EXITED;
        printf("[dbg] child exited with status %d\n", WEXITSTATUS(status));
        return 0;
    }

    if (WIFSIGNALED(status)) {
        dbg->state = EXITED;
        printf("[dbg] child terminated by signal %d\n", WTERMSIG(status));
        return 0;
    }

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



int handleBP(Debugger *dbg){
    struct user_regs_struct r;
    int status;

    ptrace(PTRACE_GETREGS, dbg->child_pid, 0, &r);

    unsigned long addr = r.rip - 1;
    int idx = findBP(addr);

    if (idx < 0)
        return 0;   // not our breakpoint

    printf("[dbg] breakpoint hit at 0x%lx\n", addr);

    // restore original instruction
    clearBP(dbg->child_pid, addr);

    // go back to real instruction
    r.rip = addr;
    ptrace(PTRACE_SETREGS, dbg->child_pid, 0, &r);

    // execute it once
    ptrace(PTRACE_SINGLESTEP, dbg->child_pid, 0, 0);
    waitpid(dbg->child_pid, &status, 0);

    // put breakpoint back
    setBP(dbg->child_pid, addr);

    return 1;
}


int launchDebugger(Debugger *dbg, char *prog, char **args){
    pid_t pid = fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(prog, args);
        perror("execvp");
        _exit(1);
    }

    waitpid(pid, NULL, 0);

    dbg->child_pid = pid;
    dbg->state = STOPPED;

    printf("[dbg] child %d stopped (ready)\n", pid);
    return 0;
}


int continueDebugger(Debugger *dbg){
    int status;
    int skipped_first_breakpoint = 0;

    while (1) {
        ptrace(PTRACE_CONT, dbg->child_pid, 0, 0);
        waitpid(dbg->child_pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("[dbg] child exited with status %d\n",
                   WEXITSTATUS(status));
            dbg->state = EXITED;
            return 0;
        }

        if (WIFSTOPPED(status)) {
            if (handleBP(dbg)) {
                if (!skipped_first_breakpoint) {
                    skipped_first_breakpoint = 1;
                    continue;   // ignore first BP
                }
            }
            dbg->state = STOPPED;
            return 0;           // stop for user
        }
    }
}
