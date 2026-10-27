#include "threadimpl.h"
#include "p9web.h"

static Lock thewaitlock;
static Channel *thewaitchan;

static void
execproc(void *v)
{
	int pid;
	Channel *c;
	Execjob *e;
	Waitmsg *w;

	e = v;
	pid = _threadspawn(e->fd, e->cmd, e->argv, e->dir);
	sendul(e->c, pid);
	if(pid > 0){
		w = waitfor(pid);
		if((c = thewaitchan) != nil)
			sendp(c, w);
		else
			free(w);
	}
	threadexits(nil);
}

int
_runthreadspawn(int *fd, char *cmd, char **argv, char *dir)
{
	int pid;
	Execjob e;

	e.fd = fd;
	e.cmd = cmd;
	e.argv = argv;
	e.dir = dir;
	e.c = chancreate(sizeof(void*), 0);
	proccreate(execproc, &e, 65536);
	pid = recvul(e.c);
	chanfree(e.c);
	return pid;
}

Channel*
threadwaitchan(void)
{
	if(thewaitchan)
		return thewaitchan;
	lock(&thewaitlock);
	if(thewaitchan){
		unlock(&thewaitlock);
		return thewaitchan;
	}
	thewaitchan = chancreate(sizeof(Waitmsg*), 4);
	chansetname(thewaitchan, "threadwaitchan");
	unlock(&thewaitlock);
	return thewaitchan;
}

/*
 * p9web: the kernel's spawn (p9web.h), not fork and exec: the child has fd[0], fd[1] and fd[2] as
 * 0, 1 and 2, and no other descriptor, in dir; cmd is looked for in $PATH, as execvp does.
 */
int
_threadspawn(int fd[3], char *cmd, char *argv[], char *dir)
{
	int pid;

	pid = p9spawndir(cmd, argv, nil, dir, fd, 3, 0);
	if(pid < 0)
		return -1;
	close(fd[0]);
	if(fd[1] != fd[0])
		close(fd[1]);
	if(fd[2] != fd[1] && fd[2] != fd[0])
		close(fd[2]);
	return pid;
}

int
threadspawn(int fd[3], char *cmd, char *argv[])
{
	return _runthreadspawn(fd, cmd, argv, nil);
}

int
threadspawnd(int fd[3], char *cmd, char *argv[], char *dir)
{
	return _runthreadspawn(fd, cmd, argv, dir);
}

int
threadspawnl(int fd[3], char *cmd, ...)
{
	char **argv, *s;
	int n, pid;
	va_list arg;

	va_start(arg, cmd);
	for(n=0; va_arg(arg, char*) != nil; n++)
		;
	n++;
	va_end(arg);

	argv = malloc(n*sizeof(argv[0]));
	if(argv == nil)
		return -1;

	va_start(arg, cmd);
	for(n=0; (s=va_arg(arg, char*)) != nil; n++)
		argv[n] = s;
	argv[n] = 0;
	va_end(arg);

	pid = threadspawn(fd, cmd, argv);
	free(argv);
	return pid;
}

int
_threadexec(Channel *cpid, int fd[3], char *cmd, char *argv[])
{
	int pid;

	pid = threadspawn(fd, cmd, argv);
	if(cpid){
		if(pid < 0)
			chansendul(cpid, ~0);
		else
			chansendul(cpid, pid);
	}
	return pid;
}

void
threadexec(Channel *cpid, int fd[3], char *cmd, char *argv[])
{
	if(_threadexec(cpid, fd, cmd, argv) >= 0)
		threadexits("threadexec");
}

void
threadexecl(Channel *cpid, int fd[3], char *cmd, ...)
{
	char **argv, *s;
	int n, pid;
	va_list arg;

	va_start(arg, cmd);
	for(n=0; va_arg(arg, char*) != nil; n++)
		;
	n++;
	va_end(arg);

	argv = malloc(n*sizeof(argv[0]));
	if(argv == nil){
		if(cpid)
			chansendul(cpid, ~0);
		return;
	}

	va_start(arg, cmd);
	for(n=0; (s=va_arg(arg, char*)) != nil; n++)
		argv[n] = s;
	argv[n] = 0;
	va_end(arg);

	pid = _threadexec(cpid, fd, cmd, argv);
	free(argv);

	if(pid >= 0)
		threadexits("threadexecl");
}
