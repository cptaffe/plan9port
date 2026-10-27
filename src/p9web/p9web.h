/*
 * The p9web extension to WASI preview 1: what a shell and mk need that WASI lacks.
 * Each import returns a value >= 0, or -errno.
 */
#ifndef _P9WEB_H
#define _P9WEB_H
#define P9IMPORT(n) __attribute__((import_module("p9web"), import_name(n)))

/* Start path with argv and env (NUL-separated lists) in cwd. Child fd i is the
 * caller's fdmap[i] (-1: closed), for i < nfd. With flags RFFDG, the child's
 * other descriptors are a copy of the caller's, but those it opened with
 * OCEXEC, as after a fork and an exec. Returns the child's pid. */
P9IMPORT("spawn") int p9web_spawn(const char *path, int pathlen, const char *argv, int argvlen,
	const char *env, int envlen, const char *cwd, int cwdlen, const int *fdmap, int nfd, int flags);
/* Replace this program with path, argv and env (NUL-separated lists; envlen 0: this
 * environment). Returns only an error. */
P9IMPORT("exec") int p9web_exec(const char *path, int pathlen, const char *argv, int argvlen, const char *env, int envlen);
/* Wait for child pid (-1: any). *status is exit<<8 | signal. flags 1: do not block. */
P9IMPORT("wait") int p9web_wait(int pid, int *status, int flags);
/* Plan 9's await: child pid's wait record (0: any child's), as a string into buf[n]; returns
 * its length. */
P9IMPORT("await") int p9web_await(int pid, char *buf, int n);
/* The status of the exit that follows (lib9's exits): Plan 9's message, not the number. */
P9IMPORT("exitmsg") void p9web_exitmsg(const char *msg, int n);
/* The text of this process's last error from the kernel into buf[n]; returns its errno (0: none). */
P9IMPORT("errstr") int p9web_errstr(char *buf, int n);
P9IMPORT("pipe") int p9web_pipe(int *fds);
P9IMPORT("dup") int p9web_dup(int fd);
P9IMPORT("dup2") int p9web_dup2(int fd, int newfd);
P9IMPORT("getpid") int p9web_getpid(void);
P9IMPORT("getppid") int p9web_getppid(void);
P9IMPORT("kill") int p9web_kill(int pid, int sig);
/* rfork(flags) with RFPROC: a copy of this process, which returns 0 in the child
 * and the child's pid here. Only a program built with asyncify can call it. */
P9IMPORT("fork") int p9web_fork(int flags);
/* rfork without RFPROC: the kernel's new groups for this process (RFNAMEG, RFENVG, RFNOTEG, ...). */
P9IMPORT("rfork") int p9web_rfork(int flags);
/* Plan 9's open and create: Plan 9's modes and permission, which WASI's path_open does not
 * carry (it has no permission). They return the descriptor. */
P9IMPORT("open") int p9web_open(const char *name, int namelen, int mode);
P9IMPORT("create") int p9web_create(const char *name, int namelen, int mode, int perm);
/* Plan 9's stat and wstat: a stat in 9P2000's form (stat(5)) into or from buf. stat
 * returns the stat's size, which may be more than n (then nothing is copied). */
P9IMPORT("stat") int p9web_stat(const char *name, int namelen, unsigned char *buf, int n);
P9IMPORT("fstat") int p9web_fstat(int fd, unsigned char *buf, int n);
P9IMPORT("wstat") int p9web_wstat(const char *name, int namelen, unsigned char *buf, int n);
P9IMPORT("fwstat") int p9web_fwstat(int fd, unsigned char *buf, int n);
/* Plan 9's chdir: the kernel's walk of path (a # device too); the new directory's name into buf;
 * returns its length. */
P9IMPORT("chdir") int p9web_chdir(const char *path, int pathlen, char *buf, int n);
/* Plan 9's bind, mount and unmount (bind(2)): the namespace of this process. */
P9IMPORT("bind") int p9web_bind(const char *name, int namelen, const char *old, int oldlen, int flag);
P9IMPORT("mount") int p9web_mount(int fd, int afd, const char *old, int oldlen, int flag, const char *aname, int anamelen);
P9IMPORT("unmount") int p9web_unmount(const char *name, int namelen, const char *old, int oldlen);
/* notify(2)'s kernel half: on, the program has a note handler (note.c). */
P9IMPORT("notify") int p9web_notify(int on);
/* The working directory, into buf; returns its length. */
P9IMPORT("getcwd") int p9web_getcwd(char *buf, int len);

int p9spawn(char *path, char **argv, char **env, int *fdmap, int nfd);
int p9spawnfd(char *path, char **argv, char **env, int *fdmap, int nfd, int flags);
/* p9spawnfd in dir (nil: this process's directory). */
int p9spawndir(char *path, char **argv, char **env, char *dir, int *fdmap, int nfd, int flags);
#endif
