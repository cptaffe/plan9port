/*
 * lib9 functions that need signals, which a p9web process has not, and rfork. Notes are
 * note.c's.
 */
#include <u.h>
#include <errno.h>
#include <unistd.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include "p9web.h"

char *_p9sigstr(int sig, char *tmp) { sprint(tmp, "signal %d", sig); return tmp; }
int _p9strsig(char *s) { USED(s); return 0; }
/* rfork: with RFPROC a fork (p9web.c's), else the kernel's rfork of this process's groups. */
int
p9rfork(int flags)
{
	int r;

	r = flags & RFPROC ? p9web_fork(flags) : p9web_rfork(flags);
	if(r < 0){
		werrstr("rfork: error %d", -r);
		return -1;
	}
	return r;
}
/* sleep: WASI's (poll_oneoff with a clock), which the runtime makes the kernel's sleep. */
int p9sleep(long ms) { return ms > 0 ? usleep(ms * 1000) : 0; }
long p9alarm(ulong ms) { USED(ms); return 0; }

/* Plan 9's bind, mount and unmount, on the kernel's (a nil name of unmount: all of old's). */
static int
nsret(int r)
{
	if(r < 0){
		errno = -r;	/* errstr and %r give the kernel's text for it (errstr.c) */
		return -1;
	}
	return r;
}
int bind(char *name, char *old, int flag) { return nsret(p9web_bind(name, strlen(name), old, strlen(old), flag)); }
int mount(int fd, int afd, char *old, int flag, char *aname) { return nsret(p9web_mount(fd, afd, old, strlen(old), flag, aname ? aname : "", aname ? strlen(aname) : 0)); }
int unmount(char *name, char *old) { return nsret(p9web_unmount(name ? name : "", name ? strlen(name) : 0, old, strlen(old))); }
