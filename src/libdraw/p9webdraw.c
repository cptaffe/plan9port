/*
 * p9web: libdraw's transport on the kernel's files, in place of drawclient.c and drawfcall.c
 * (devdraw's RPCs over a pipe): /dev/draw (draw(3), the kernel's #i), and the window's
 * /dev/mouse, /dev/cursor, /dev/cons and /dev/consctl, /dev/label, /dev/snarf and /dev/wctl
 * (the kernel's #m), as Plan 9's libdraw uses them. Each libthread proc reads its own file (the
 * mouse proc /dev/mouse, the keyboard proc /dev/cons), so there is no mux.
 */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>
#include <mouse.h>

extern Mouse _drawmouse;	/* init.c's: the last state, for its visible clicks */

static int ctlfd = -1;
static char ctlinfo[12*12];	/* the ctl's first read: the screen, image 0 */
static int mousefd = -1, consfd = -1, consctlfd = -1, raw;
static Lock fdlock;

/* openonce opens name with mode into *fd, the first time. */
static int
openonce(int *fd, char *name, int mode)
{
	lock(&fdlock);
	if(*fd < 0)
		*fd = open(name, mode);
	unlock(&fdlock);
	return *fd;
}

int
_displaymux(Display *d)
{
	USED(d);
	return 0;
}

int
_displayconnect(Display *d)
{
	char buf[64];

	if((ctlfd = open("/dev/draw/new", ORDWR|OCEXEC)) < 0)
		return -1;
	if(read(ctlfd, ctlinfo, sizeof ctlinfo) != sizeof ctlinfo){
		werrstr("short read of /dev/draw/new: %r");
		close(ctlfd);
		return -1;
	}
	d->dirno = atoi(ctlinfo);
	snprint(buf, sizeof buf, "/dev/draw/%d/data", d->dirno);
	if((d->srvfd = open(buf, ORDWR|OCEXEC)) < 0){
		close(ctlfd);
		return -1;
	}
	return 0;
}

int
_displayinit(Display *d, char *label, char *winsize)
{
	USED(winsize);	/* the page's canvas has its size */
	return label ? _displaylabel(d, label) : 0;
}

/*
 * _displayscreen gives the screen's ctl information (12 fields of 12 characters) as image 0:
 * at the first attach, the ctl's first read; at a reattach (after a resize), the window's new
 * screen, by its name in /dev/winname, named as image 0 with 'n' (9front's gengetwindow).
 */
int
_displayscreen(Display *d, char *info, int reattach)
{
	char name[64];
	uchar *a;
	int fd, n;

	if(!reattach){
		memmove(info, ctlinfo, sizeof ctlinfo);
		return 0;
	}
	if((fd = open("/dev/winname", OREAD)) < 0)
		return -1;
	n = read(fd, name, sizeof name - 1);
	close(fd);
	if(n <= 0){
		werrstr("no /dev/winname");
		return -1;
	}
	a = bufimage(d, 1+4+1+n);
	if(a == nil)
		return -1;
	a[0] = 'n';
	BPLONG(a+1, 0);
	a[5] = n;
	memmove(a+6, name, n);
	if(flushimage(d, 0) < 0)
		return -1;
	if(read(ctlfd, info, 12*12) != 12*12){
		werrstr("short read of the screen's ctl: %r");
		return -1;
	}
	return 0;
}

/* /dev/mouse: "m" or "r" (a resize), then x, y, buttons, msec, each %11d and a space. */
int
_displayrdmouse(Display *d, Mouse *m, int *resized)
{
	char buf[49];
	int n;

	USED(d);
	if(openonce(&mousefd, "/dev/mouse", ORDWR) < 0)
		return -1;
	if((n = read(mousefd, buf, sizeof buf)) != sizeof buf){
		if(n >= 0)
			werrstr("short read of /dev/mouse");
		return -1;
	}
	*resized = buf[0] == 'r';
	m->xy.x = atoi(buf+1+0*12);
	m->xy.y = atoi(buf+1+1*12);
	m->buttons = atoi(buf+1+2*12);
	m->msec = strtoul(buf+1+3*12, 0, 10);
	_drawmouse = *m;
	return 0;
}

/* /dev/cons in raw mode: a rune from its UTF-8 bytes (9front's keyboard.c). */
int
_displayrdkbd(Display *d, Rune *r)
{
	char buf[UTFmax];
	int n;

	USED(d);
	if(openonce(&consctlfd, "/dev/consctl", OWRITE) < 0 || openonce(&consfd, "/dev/cons", OREAD) < 0)
		return -1;
	lock(&fdlock);
	if(!raw && write(consctlfd, "rawon", 5) == 5)	/* consctl stays open: raw mode lasts */
		raw = 1;
	unlock(&fdlock);
	for(n=0; n<UTFmax; ){
		if(read(consfd, buf+n, 1) != 1)
			return -1;
		n++;
		if(fullrune(buf, n))
			break;
	}
	chartorune(r, buf);
	return 0;
}

int
_displaymoveto(Display *d, Point p)
{
	char buf[64];
	int n;

	if(openonce(&mousefd, "/dev/mouse", ORDWR) < 0)
		return -1;
	n = snprint(buf, sizeof buf, "m%d %d", p.x, p.y);
	if(write(mousefd, buf, n) != n)
		return -1;
	_drawmouse.xy = p;
	return flushimage(d, 1);
}

/* /dev/cursor: Plan 9's 72 bytes (the offset's two longs, clr, set), then the 32x32 Cursor2's
 * clr and set; none for the arrow. */
int
_displaycursor(Display *d, Cursor *c, Cursor2 *c2)
{
	uchar buf[2*4+2*32+2*128];
	Cursor2 s;
	int fd, n;

	USED(d);
	n = 0;
	if(c != nil){
		if(c2 == nil){
			scalecursor(&s, c);
			c2 = &s;
		}
		BPLONG(buf+0, c->offset.x);
		BPLONG(buf+4, c->offset.y);
		memmove(buf+8, c->clr, 32);
		memmove(buf+40, c->set, 32);
		memmove(buf+72, c2->clr, 128);
		memmove(buf+200, c2->set, 128);
		n = sizeof buf;
	}
	if((fd = open("/dev/cursor", OWRITE)) < 0)
		return -1;
	n = write(fd, buf, n) == n ? 0 : -1;
	close(fd);
	return n;
}

int
_displaybouncemouse(Display *d, Mouse *m)
{
	USED(d);
	USED(m);
	return 0;
}

static int
writefile(char *name, char *s, int n)
{
	int fd, r;

	if((fd = open(name, OWRITE|OTRUNC)) < 0)
		return -1;
	r = write(fd, s, n) == n ? 0 : -1;
	close(fd);
	return r;
}

int
_displaylabel(Display *d, char *label)
{
	USED(d);
	return writefile("/dev/label", label, strlen(label));
}

char*
_displayrdsnarf(Display *d)
{
	char *s;
	int fd, n, m;

	USED(d);
	if((fd = open("/dev/snarf", OREAD)) < 0)
		return nil;
	s = nil;
	for(n=0;; n+=m){
		s = realloc(s, n+8192+1);
		if(s == nil || (m = read(fd, s+n, 8192)) <= 0)
			break;
	}
	close(fd);
	if(s != nil)
		s[n] = 0;
	return s;
}

int
_displaywrsnarf(Display *d, char *snarf)
{
	USED(d);
	return writefile("/dev/snarf", snarf, strlen(snarf));
}

int
_displayrddraw(Display *d, void *v, int n)
{
	return read(d->srvfd, v, n);
}

int
_displaywrdraw(Display *d, void *v, int n)
{
	return write(d->srvfd, v, n);
}

int
_displaytop(Display *d)
{
	USED(d);
	return writefile("/dev/wctl", "current", 7);
}

/* The page's canvas has its size: a program cannot resize it. */
int
_displayresize(Display *d, Rectangle r)
{
	USED(d);
	USED(r);
	return 0;
}
