#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h> 
#include "launch.h"

/* Fork and exec the target program.
 *
 * Child: replaces its image with path_argv[0] via execvp().
 * Parent: returns child's pid (or -1 on error).
 *
 * This function does not trace or ptrace the child.
 */
pid_t dbg_launch(char *path_argv[]) {
    if (path_argv == NULL || path_argv[0] == NULL) {
       
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        /* fork failed */
        return -1;
    }

    if (pid == 0) {
        /* Child process: replace image with target program */


        // tell the kernal that ready to be trwced

        if(ptrace(PTRACE_TRACEME , 0 , NULL , NULL) == -1){
            perror("ptrace_traceme(launch.c)");
            _exit(127);
        }
        


        execvp(path_argv[0], path_argv);
        /* If execvp returns, an error occurred */
        perror("execvp(launch.c)");
        _exit(127); 
    }

    /* Parent process: return child's pid (caller may wait/reap) */
    return pid;
}
