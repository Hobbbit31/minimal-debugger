#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "include/debug.h"




static void print_help(void)
{
    puts("commands:");
    puts("  run <program>   start a program under debugger");
    puts("  continue        resume execution");
    puts("  step            execute one instruction");
    puts("  quit            exit debugger");
}

int main(void)
{
    Debugger dbg;
    dbg.child_pid = -1;
    dbg.state = NOT_STARTED;

    char line[128];

    while (1) {
        printf("mydbg> ");
        fflush(stdout);

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

        //run
        if (strncmp(line, "run ", 4) == 0) {
            if (dbg.state == RUNNING || dbg.state == STOPPED) {
                printf("[dbg] program already started\n");
                continue;
            }
            intializationBP();               // reset breakpoint table
            dbg.child_pid = -1;
            dbg.state = NOT_STARTED;

            char *prog = line + 4;
            char *args[] = { prog, NULL };

            if (launchDebugger(&dbg, prog, args) == 0) {
                dbg.state = STOPPED;
                addBP(0x40113e);
                addBP(0x40115f);

                setBP(dbg.child_pid, 0x40113e);
                setBP(dbg.child_pid, 0x40115f);
                // continueDebugger(&dbg);   // SIGTRAP
                
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
