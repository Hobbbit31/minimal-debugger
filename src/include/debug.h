#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <sys/types.h>

typedef enum {
    NOT_STARTED,   // no program loaded
    STOPPED,       // child exists, currently stopped
    RUNNING,       // child running
    EXITED         // child exited
} dbg_state_t;

typedef struct {
    pid_t child_pid;
    dbg_state_t state;
} Debugger;

int launchDebugger(Debugger *dbg, char *prog, char **args);
int continueDebugger(Debugger *dbg);

int debuggerStep(Debugger *dbg);

int handleBP(Debugger *dbg);

void printRegisters(Debugger *dbg);

#endif
