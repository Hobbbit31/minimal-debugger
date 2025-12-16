#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/debug.h"
#include "include/breakpoint.h"


void print_help(){
    puts("commands:");
    puts("  run <program>       start a program under debugger");
    puts("  continue            resume execution");
    puts("  break  <hexaddr>    set breakpoint at address");
    puts("  delete <hexaddr>    remove breakpoint");
    puts("  step                execute one instruction");
    puts("  quit                exit debugger");
}

int main(){
    Debugger dbg;
    dbg.child_pid = -1;
    dbg.state = NOT_STARTED;
    dbg.lastStatus = 0; 

    char line[128];
    print_help();


    while (1) {
        printf("mydbg> ");
        fflush(stdout);

        //read user input
        if (!fgets(line, sizeof(line), stdin))
            break;

        //remove emoty or new line
        line[strcspn(line, "\n")] = 0;

        //quir
        if (strcmp(line, "quit") == 0) {
            break;
        }

        //help
        if (strcmp(line, "help") == 0) {
            print_help();
            continue;
        }
        
        // process status
        if(strcmp(line, "status") == 0){
            printProcessStatus(&dbg);
            continue;
        }

        //run
        if (strncmp(line, "run ", 4) == 0) {
            if (dbg.state == RUNNING || dbg.state == STOPPED) {
                printf("[dbg](run,main.c) program already started\n");
                continue;
            }

            // Reset breakpoint table for new program 
            intializationBP();               
            dbg.child_pid = -1;
            dbg.state = NOT_STARTED;

            // Extract program name
            char *prog = line + 4;
            char *args[] = { prog, NULL };
            if (launchDebugger(&dbg, prog, args) == 0) {
                dbg.state = STOPPED;
            }
            continue;
        }

        // break
        if (strncmp(line, "break ", 6) == 0) {
            if (dbg.state == NOT_STARTED) {
                printf("[dbg](break,main.c) program not started\n");
                continue;
            }

            char *end;

            unsigned long addr = strtoul(line + 6, &end, 16);

            // Validate hexadecimal address
            if(end == line + 6){
                printf("[dbg](break,main.c) invalid add \n");
                continue;
            }

            // Add breakpoint to internal table
            int ret = addBP(addr);
            if (ret == 0) {
                // Insert INT3 only if program is stopped 
                if (dbg.state == STOPPED) {
                    setBP(dbg.child_pid, addr);
                }
                printf("[dbg](break,main.c) breakpoint set at 0x%lx\n", addr);
            } else if(ret == -2){
                printf("[dbg](break,main.c) breakpoint already exists at 0x%lx\n", addr);
            }else{
                printf("[dbg](break,main.c) failed to add breakpoint\n");
            }
            continue;
        }

        // print the regs
        if(strncmp(line , "regs" ,4) == 0){
            if (dbg.state != STOPPED) {
                if (dbg.state == EXITED) {
                    printf("[dbg](regs,main.c) process has exited\n");
                } else {
                    printf("[dbg](regs,main.c) process is not stopped\n");
                }
                continue;
            }

            state mode = ALL;

            if(strcmp(line, "regs -s") == 0) {
                mode = STACK;
            } else if(strcmp(line, "regs -g") == 0) {
                mode = GENERAL;
            }else if (strcmp(line, "regs") != 0) {
                printf("[dbg](regs,main.c) usage: regs [-s|-g]\n");
            continue;
            }

            printRegisters(&dbg, mode);
            continue;
        }
        
        // delete
        if (strncmp(line, "delete ", 7) == 0) {
            if (dbg.state == NOT_STARTED) {
                printf("[dbg](delete,main.c) program not started\n");
                continue;
            }

            char *end;
            unsigned long addr = strtoul(line + 7, &end, 16);

            if(end == line+7){
                printf("[dbg](delete,main.c) invalid address\n");
                continue;
            }

            // restore instruction if needed, Never modify debuggee memory while it is RUNNING 
            // if (dbg.state == STOPPED ) {
            //     clearBP(dbg.child_pid, addr);
            // }

            if (dbg.state == RUNNING) {
                stopDebugger(&dbg);
            }
            clearBP(dbg.child_pid, addr);

            if (removeBP(addr) == 0) {
                printf("[dbg](delete,main.c) breakpoint removed at 0x%lx\n", addr);
            } else {
                printf("[dbg](delete,main.c) breakpoint not found\n");
            }
            continue;
        }

        //comtinue
        if(strcmp(line, "continue") == 0) {
            if(dbg.state == EXITED) {
                printf("[dbg](continue,main.c) program has already exited\n");
            }else {
                continueDebugger(&dbg);
            }
            continue;
        }

        // for step
        if (strcmp(line, "step") == 0) {
            debuggerStep(&dbg);
            continue;
        }

        printf("[dbg](main.c) unknown command: %s\n", line);
    }

    return 0;
}
