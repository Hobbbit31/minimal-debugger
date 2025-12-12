#ifndef WAIT_H
#define WAIT_H

#include <sys/types.h>

typedef enum {
    no_event = 0,
    stop_event,   //(WIFSTOPPED) 
    exit_event,   //(WIFEXITED) 
    signaled_event //(WIFSIGNALED)
}type_of_event;




//dbg_event describes what happened to the child
struct event{
    type_of_event type;
    pid_t pid;    // child pid 
    int sig;     // signal number for STOP or SIGNALED 
    int exit_code;
};

// Wait for a state change in `pid` and fill `ev`.
//  Returns 0 on success (ev filled), -1 on error (errno set).
//  This call blocks until the child changes state (stop/exit).
int dbg_wait(pid_t pid, struct event *ev);

#endif 
