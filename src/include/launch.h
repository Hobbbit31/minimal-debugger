#ifndef LAUNCH_H
#define LAUNCH_H

#include <sys/types.h>

/* Launch a target program.
 *
 * path_argv is the argv array passed to execvp: e.g. { "ls", "-l", NULL }
 *
 * Returns:
 *  - on success: child's pid (> 0)
 *  - on error: -1 (errno is set)
 *
 * Note: this function does not use ptrace. It simply forks and execs.
 */
pid_t dbg_launch(char **path_argv);

#endif 
