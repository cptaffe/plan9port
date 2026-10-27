/*
 * p9web: threadpostmountsrv as Plan 9's, on the kernel's mount, in place of tpost.c (which gives
 * the pipe to 9pserve): the server's end of a pipe is posted in /srv as name, and mounted on
 * mtpt with flag. The server's proc starts first: the mount waits for its answers.
 */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

static void
launchsrv(void *v)
{
	srv(v);
}

void
threadpostmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	int fd[2], sfd;
	char buf[128];

	if(!s->nopipe){
		if(pipe(fd) < 0)
			sysfatal("pipe: %r");
		s->infd = s->outfd = fd[1];
		s->srvfd = fd[0];
	}
	if(name == nil && mtpt == nil && !s->nopipe)
		sysfatal("no one to serve");
	if(s->foreground)
		sysfatal("threadpostmountsrv: a foreground server cannot mount itself");
	proccreate(launchsrv, s, 32*1024);
	if(name){
		snprint(buf, sizeof buf, "/srv/%s", name);
		if((sfd = create(buf, OWRITE|ORCLOSE, 0600)) < 0 || fprint(sfd, "%d", s->srvfd) < 0)
			sysfatal("post %s: %r", buf);
	}
	if(mtpt && mount(s->srvfd, -1, mtpt, flag, "") < 0)
		sysfatal("mount %s: %r", mtpt);
}
