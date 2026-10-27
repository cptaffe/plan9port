/* p9web: wait(2) for WASI, served by the kernel's p9web.wait. */
#ifndef _P9WEB_SYS_WAIT_H
#define _P9WEB_SYS_WAIT_H
#include <sys/types.h>
#include <sys/resource.h>
#define WNOHANG 1
#define WIFEXITED(s) (((s) & 0x7f) == 0)
#define WEXITSTATUS(s) (((s) >> 8) & 0xff)
#define WIFSIGNALED(s) (((s) & 0x7f) != 0 && ((s) & 0x7f) != 0x7f)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFSTOPPED(s) 0
#define WSTOPSIG(s) 0
#define WCOREDUMP(s) 0
pid_t waitpid(pid_t, int*, int);
pid_t wait4(pid_t, int*, int, struct rusage*);
pid_t wait(int*);
#endif
