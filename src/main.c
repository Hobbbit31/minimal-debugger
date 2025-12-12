#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include "include/launch.h"
#include "include/core.h"
#include "include/wait.h"


char *nameofSignal(int signal) {
    switch (signal) {
        case SIGTRAP: return "SIGTRAP";
        case SIGSEGV: return "SIGSEGV";
        case SIGINT:  return "SIGINT";
        case SIGTERM: return "SIGTERM";
        case SIGCHLD: return "SIGCHLD";
        case SIGABRT: return "SIGABRT";
        default: return "UNKNOWN";
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("error(main.c): less than two argumen not allowed.\n");
        return 0;
    }


    char  **target = &argv[1];

    pid_t child = dbg_launch(target);

    if(child < 0){
        perror("dbg_launch");
        return 1;
    }

    printf("child pid which is launched: %d\n" , (int)child);
    struct event *ev;

    if(dbg_wait(child , ev) < 0){

        perror("wait(main.c):");
        return 1;

    }

    if(ev->type == stop_event){
        printf("singalled stop %d (%s) \n", ev->sig , nameofSignal(ev->sig));
    }else if(ev->type == exit_event){
        printf("child exited immmediately %d \n", ev->exit_code);
    }else if(ev->type == signaled_event){
        printf("child terminated by signal event this %d (%s) \n", ev->sig , nameofSignal(ev->sig));
    }else{
        printf("child state %d", ev->type);
    }

   

    return 0;
}
