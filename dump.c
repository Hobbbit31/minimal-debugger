
// int launchDebugger(Debugger *dbg, char *prog, char **args){
//     pid_t pid = fork();

//     if (pid < 0) {
//         perror("fork");
//         return -1;
//     }

//     if (pid == 0) {
//         /* -------- CHILD (debuggee) -------- */

//         if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
//             perror("ptrace TRACEME");
//             _exit(1);
//         }

//         execvp(prog, args);

//         /* exec only returns on failure */
//         perror("execvp");
//         _exit(1);
//     }

//     /* -------- PARENT (debugger) -------- */

//     int status;
//     if (waitpid(pid, &status, 0) == -1) {
//         perror("waitpid");
//         return -1;
//     }

//     if (WIFSTOPPED(status)) {
//         dbg->child_pid = pid;
//         dbg->state = STOPPED;
//         printf("[dbg] child %d stopped (ready for debugging)\n", pid);
//         return 0;
//     }

//     fprintf(stderr, "[dbg] unexpected child state\n");
//     return -1;
// }


// int continueDebugger(Debugger *dbg){
//     int status;
//     int auto_continued = 0;   // 🔑 NEW

//     if (dbg->state != STOPPED) {
//         fprintf(stderr, "[dbg] cannot continue\n");
//         return -1;
//     }

//     while (1) {
//         ptrace(PTRACE_CONT, dbg->child_pid, NULL, NULL);
//         dbg->state = RUNNING;

//         waitpid(dbg->child_pid, &status, 0);

//         if (WIFEXITED(status)) {
//             dbg->state = EXITED;
//             printf("[dbg] child exited with status %d\n",
//                    WEXITSTATUS(status));
//             return 0;
//         }

//         if (WIFSTOPPED(status)) {
//             int sig = WSTOPSIG(status);

//             if (sig == SIGTRAP) {
//                 if (handleBP(dbg)) {
//                     if (!auto_continued) {
//                         // 🔑 auto-continue ONLY ONCE
//                         auto_continued = 1;
//                         continue;
//                     }
//                 }
//             }

//             // 🔑 user-visible stop
//             dbg->state = STOPPED;
//             return 0;
//         }
//     }
// }


// int handleBP(Debugger *dbg){
//     struct user_regs_struct regs;
//     int status;

//     /* read registers */
//     if (ptrace(PTRACE_GETREGS, dbg->child_pid, NULL, &regs) == -1) {
//         perror("ptrace GETREGS");
//         return -1;
//     }

//     /* INT3 increments RIP by 1 */
//     unsigned long bp_addr = regs.rip - 1;

//     int idx = findBP(bp_addr);
//     if (idx < 0) {
//         /* SIGTRAP not caused by our breakpoint */
//         return 0;
//     }

//     printf("[dbg] breakpoint hit at 0x%lx\n", bp_addr);

//     /* restore original instruction */
//     clearBP(dbg->child_pid, bp_addr);

//     /* fix instruction pointer */
//     regs.rip = bp_addr;

//     if (ptrace(PTRACE_SETREGS, dbg->child_pid, NULL, &regs) == -1) {
//         perror("ptrace SETREGS");
//         return -1;
//     }
//     /* 3. single-step one instruction */
//     if (ptrace(PTRACE_SINGLESTEP, dbg->child_pid, NULL, NULL) == -1) {
//         perror("ptrace SINGLESTEP");
//         return -1;
//     }

//     if (waitpid(dbg->child_pid, &status, 0) == -1) {
//         perror("waitpid");
//         return -1;
//     }

//     /* 4. reinsert breakpoint */
//     setBP(dbg->child_pid, bp_addr);

//     dbg->state = STOPPED;


//     return 1;  // breakpoint handled
// }
 


//upper file is from debugger.c