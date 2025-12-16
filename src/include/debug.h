#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <sys/types.h>

typedef enum {
    NOT_STARTED,   // no program loaded
    STOPPED,       // child exists, currently stopped
    RUNNING,       // child running
    EXITED         // child exited
} dbgstate;

typedef struct {
    pid_t child_pid;
    dbgstate state;
    int lastStatus;
   
} Debugger;


typedef enum{
    ALL,
    STACK,
    GENERAL
}state;

int launchDebugger(Debugger *dbg, char *prog, char **args);

int continueDebugger(Debugger *dbg);

int debuggerStep(Debugger *dbg);

int handleBP(Debugger *dbg);

void printRegisters(Debugger *dbg ,state st);

unsigned long logicalrip(unsigned long real_rip);

void printProcessStatus(Debugger *dbg);

int stopDebugger(Debugger *dbg);

#endif


// signal 5 is SIGTRAP