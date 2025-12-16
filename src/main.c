#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/debug.h"
#include "include/breakpoint.h"


void print_help(void){
    puts("commands:");
    puts("  run <program>   start a program under debugger");
    puts("  continue        resume execution");
    puts("  break  <hexaddr>    set breakpoint at address");
    puts("  delete <hexaddr>   remove breakpoint");
    puts("  step            execute one instruction");
    puts("  quit            exit debugger");
}

int main(){
    Debugger dbg;
    dbg.child_pid = -1;
    dbg.state = NOT_STARTED;
    dbg.lastStatus = 0;

    char line[128];


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
                printf("[dbg] program already started\n");
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
                printf("[dbg] program not started\n");
                continue;
            }

            char *end;

            unsigned long addr = strtoul(line + 6, &end, 16);

            // Validate hexadecimal address
            if(end == line + 6){
                printf("[dbg] invalid add \n");
                continue;
            }

            // Add breakpoint to internal table
            if (addBP(addr) == 0) {
                // Insert INT3 only if program is stopped 
                if (dbg.state == STOPPED) {
                    setBP(dbg.child_pid, addr);
                }
                printf("[dbg] breakpoint set at 0x%lx\n", addr);
            } else {
                printf("[dbg] failed to add breakpoint\n");
            }
            continue;
        }

        // print the regs
        if(strncmp(line , "regs" ,4) == 0){
            if (dbg.state != STOPPED) {
                printf("[dbg] process is not stopped\n");
                continue;
            }

            state mode = ALL;

            if(strcmp(line, "regs -s") == 0) {
                mode = STACK;
            } else if(strcmp(line, "regs -g") == 0) {
                mode = GENERAL;
            }else if (strcmp(line, "regs") != 0) {
                printf("[dbg] usage: regs [-s|-g]\n");
            continue;
            }

            printRegisters(&dbg, mode);
            continue;
        }
        
        // delete
        if (strncmp(line, "delete ", 7) == 0) {
            if (dbg.state == NOT_STARTED) {
                printf("[dbg] program not started\n");
                continue;
            }

            char *end;
            unsigned long addr = strtoul(line + 7, &end, 16);

            if(end == line+7){
                printf("[dbg] invalid address\n");
                continue;
            }

            // restore instruction if needed, Never modify debuggee memory while it is RUNNING 
            if (dbg.state == STOPPED ) {
                clearBP(dbg.child_pid, addr);
            }

            if (removeBP(addr) == 0) {
                printf("[dbg] breakpoint removed at 0x%lx\n", addr);
            } else {
                printf("[dbg] breakpoint not found\n");
            }
            continue;
        }

        //comtinue
        if(strcmp(line, "continue") == 0) {
            if(dbg.state == EXITED) {
                printf("[dbg] program has already exited\n");
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

        printf("[dbg] unknown command: %s\n", line);
    }

    return 0;
}
