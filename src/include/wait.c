#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "wait.h"

int dbg_wait(pid_t pid, struct event *ev) {
    if (ev == NULL) {
        errno = EINVAL;
        return -1;
    }

    int status;
    pid_t w = waitpid(pid, &status, 0);
    if (w < 0) {
        return -1; /* errno set by waitpid */
    }

    ev->pid = w;
    ev->sig = 0;
    ev->exit_code = -1;
    ev->type = no_event;

    if (WIFSTOPPED(status)) {
        ev->type = stop_event;
        ev->sig = WSTOPSIG(status);
        return 0;
    }

    if (WIFEXITED(status)) {
        ev->type = exit_event;
        ev->exit_code = WEXITSTATUS(status);
        return 0;
    }

    if (WIFSIGNALED(status)) {
        ev->type = signaled_event;
        ev->sig = WTERMSIG(status);
        return 0;
    }

    /* Unknown state — still return success but kind NONE */
    return 0;
}
