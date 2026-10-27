/*
 * POSIX calls that wasi-libc lacks, on the p9web imports.
 */
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include "p9web.h"

extern char **environ;

static int
ret(int r)
{
	if(r < 0){
		errno = -r;
		return -1;
	}
	return r;
}

/* Start in the directory the parent gave: wasi-libc keeps the cwd itself. */
__attribute__((constructor)) static void
p9web_init(void)
{
	char buf[1024];
	int n;

	n = p9web_getcwd(buf, sizeof buf - 1);
	if(n > 0 && n < (int)sizeof buf){
		buf[n] = 0;
		chdir(buf);
	}
}

static char*
join(char **v, int *len)
{
	int n, i;
	char *s, *p;

	n = 0;
	for(i = 0; v && v[i]; i++)
		n += strlen(v[i]) + 1;
	s = malloc(n + 1);
	p = s;
	for(i = 0; v && v[i]; i++){
		strcpy(p, v[i]);
		p += strlen(v[i]) + 1;
	}
	*len = n;
	return s;
}

int
p9spawn(char *path, char **argv, char **env, int *fdmap, int nfd)
{
	return p9spawnfd(path, argv, env, fdmap, nfd, 0);
}

int
p9spawnfd(char *path, char **argv, char **env, int *fdmap, int nfd, int flags)
{
	return p9spawndir(path, argv, env, 0, fdmap, nfd, flags);
}

int
p9spawndir(char *path, char **argv, char **env, char *dir, int *fdmap, int nfd, int flags)
{
	char *a, *e, cwd[1024];
	int an, en, n, r;

	a = join(argv, &an);
	e = join(env ? env : environ, &en);
	/* The kernel's directory, which a # device can be; wasi-libc's copy cannot. */
	n = p9web_getcwd(cwd, sizeof cwd - 1);
	if(n <= 0 || n >= (int)sizeof cwd)
		strcpy(cwd, "/");
	else
		cwd[n] = 0;
	if(dir)
		snprintf(cwd, sizeof cwd, "%s", dir);
	r = p9web_spawn(path, strlen(path), a, an, e, en, cwd, strlen(cwd), fdmap, nfd, flags);
	free(a);
	free(e);
	return ret(r);
}

int pipe(int fd[2]) { return ret(p9web_pipe(fd)); }
int dup(int fd) { return ret(p9web_dup(fd)); }
int dup2(int fd, int nfd) { return ret(p9web_dup2(fd, nfd)); }
pid_t getpid(void) { return p9web_getpid(); }
pid_t getppid(void) { return p9web_getppid(); }
int kill(pid_t pid, int sig) { return ret(p9web_kill(pid, sig)); }
int killpg(pid_t pg, int sig) { return ret(p9web_kill(-pg, sig)); }
pid_t getpgid(pid_t pid) { return pid ? pid : getpid(); }
pid_t getpgrp(void) { return getpid(); }
int setpgid(pid_t a, pid_t b) { return 0; }

pid_t
wait4(pid_t pid, int *status, int flags, struct rusage *ru)
{
	int st, r;

	r = p9web_wait(pid, &st, flags & WNOHANG);
	if(r < 0)
		return ret(r);
	if(status)
		*status = st;
	if(ru)
		memset(ru, 0, sizeof *ru);
	return r;
}
pid_t waitpid(pid_t pid, int *status, int flags) { return wait4(pid, status, flags, 0); }
pid_t wait(int *status) { return wait4(-1, status, 0, 0); }

/* fork is Plan 9's: rfork(RFFDG|RFPROC). */
pid_t
fork(void)
{
	int r = p9web_fork((1<<2)|(1<<4));
	if(r < 0){
		errno = -r;
		return -1;
	}
	return r;
}

/* chdir is the kernel's, so a name it walks (a # device too) is a directory here. wasi-libc's
 * own copy of the directory, which its path calls resolve against, follows; a device has no
 * name it can resolve, so those calls stay at the last directory they had. wasi-libc resolves
 * against its copy only when chdir.c's resolver is linked (posix.c's reference to it is weak):
 * the reference below links it. */
extern char *__wasilibc_cwd;
extern int __wasilibc_find_relpath_alloc(const char *, const char **, char **, size_t *, int);
__attribute__((used)) static void *link_relpath = (void*)__wasilibc_find_relpath_alloc;

int
chdir(const char *path)
{
	static char buf[1024];
	int r;

	r = p9web_chdir(path, strlen(path), buf, sizeof buf - 1);
	if(r < 0){
		errno = -r;
		return -1;
	}
	buf[r] = 0;
	if(buf[0] == '/')
		__wasilibc_cwd = strdup(buf);
	return 0;
}

/* exec replaces this program, in this process (Plan 9's exec, with an environment). */
int
execve(const char *path, char *const argv[], char *const env[])
{
	char *a, *e;
	int an, en, r;

	fflush(0);
	a = join((char**)argv, &an);
	e = join((char**)(env ? env : environ), &en);
	r = p9web_exec(path, strlen(path), a, an, e, en);
	free(a);
	free(e);
	return ret(r);
}
int execv(const char *path, char *const argv[]) { return execve(path, argv, environ); }

uid_t getuid(void) { return 0; }
uid_t geteuid(void) { return 0; }
gid_t getgid(void) { return 0; }
gid_t getegid(void) { return 0; }

static struct passwd pw = { "none", "/", 0, 0, "/bin/rc" };
static struct group gr = { "none", 0 };
struct passwd *getpwuid(uid_t u) { return &pw; }
struct passwd *getpwnam(const char *n) { return &pw; }
struct group *getgrgid(gid_t g) { return &gr; }
pid_t wait3(int *status, int flags, struct rusage *ru) { return wait4(-1, status, flags, ru); }

/* popen and system run /bin/sh -c, as POSIX says, through spawn. */
#include <stdio.h>
#include <fcntl.h>

static struct { FILE *f; int pid; } popens[16];

FILE*
popen(const char *cmd, const char *mode)
{
	int p[2], fdmap[3] = {0, 1, 2}, pid, i, w;
	char *argv[] = {"sh", "-c", (char*)cmd, 0};
	FILE *f;

	w = mode[0] == 'w';
	if(pipe(p) < 0)
		return 0;
	fdmap[w ? 0 : 1] = p[w ? 0 : 1];
	pid = p9spawn("/bin/sh", argv, 0, fdmap, 3);
	close(p[w ? 0 : 1]);
	if(pid < 0){
		close(p[w ? 1 : 0]);
		return 0;
	}
	f = fdopen(p[w ? 1 : 0], mode);
	for(i = 0; i < 16; i++)
		if(popens[i].f == 0){
			popens[i].f = f;
			popens[i].pid = pid;
			break;
		}
	return f;
}

int
pclose(FILE *f)
{
	int i, st, pid;

	pid = -1;
	for(i = 0; i < 16; i++)
		if(popens[i].f == f){
			pid = popens[i].pid;
			popens[i].f = 0;
		}
	fclose(f);
	if(pid < 0 || waitpid(pid, &st, 0) < 0)
		return -1;
	return st;
}

int
system(const char *cmd)
{
	int fdmap[3] = {0, 1, 2}, pid, st;
	char *argv[] = {"sh", "-c", (char*)cmd, 0};

	if(cmd == 0)
		return 1;
	fflush(0);
	pid = p9spawn("/bin/sh", argv, 0, fdmap, 3);
	if(pid < 0 || waitpid(pid, &st, 0) < 0)
		return -1;
	return st;
}

/* The stack that Binaryen's asyncify saves into, for the asyncify executor. */
__attribute__((export_name("p9web_async_buf"))) char*
p9web_async_buf(void)
{
	static char buf[65536] __attribute__((aligned(16)));
	return buf;
}
