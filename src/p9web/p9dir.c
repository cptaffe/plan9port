/*
 * lib9's open, create, dirstat, dirfstat, dirwstat and dirfwstat on the kernel's own calls:
 * Plan 9's modes and permissions, and the Plan 9 Dir with its mode, owners and qid, which
 * wasi-libc's open and stat do not carry.
 */
#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include <fcall.h>
#include <errno.h>
#include "p9web.h"

/* kerr sets the errstr of the kernel's error e: its own text, which strerror's is not. */
static void
kerr(int e)
{
	char buf[ERRMAX];

	errno = e;
	if(p9web_errstr(buf, sizeof buf) == e)
		werrstr("%s", buf);
	else
		werrstr("%s", strerror(e));
}

static int
ret(int r)
{
	if(r < 0){
		kerr(-r);
		return -1;
	}
	return r;
}

int p9open(char *name, int mode) { return ret(p9web_open(name, strlen(name), mode)); }
int p9create(char *name, int mode, ulong perm) { return ret(p9web_create(name, strlen(name), mode, perm)); }

static Dir*
todir(int (*get)(void*, uchar*, int), void *a)
{
	uchar *buf;
	int n, nbuf;
	Dir *d;

	for(nbuf = 512;; nbuf = n){
		buf = malloc(nbuf);
		if(buf == nil)
			return nil;
		n = get(a, buf, nbuf);
		if(n < 0){
			free(buf);
			kerr(-n);
			return nil;
		}
		if(n <= nbuf)
			break;
		free(buf);	/* too small: n is the size it needs */
	}
	d = malloc(sizeof(Dir) + n);
	if(d == nil || convM2D(buf, n, d, (char*)&d[1]) != n){
		free(buf);
		free(d);
		werrstr("bad stat");
		return nil;
	}
	free(buf);
	return d;
}

static int getname(void *a, uchar *b, int n) { return p9web_stat(a, strlen(a), b, n); }
static int getfd(void *a, uchar *b, int n) { return p9web_fstat(*(int*)a, b, n); }

Dir* dirstat(char *name) { return todir(getname, name); }
Dir* dirfstat(int fd) { return todir(getfd, &fd); }

static int
put(Dir *d, char *name, int fd)
{
	uchar *buf;
	int n, r;

	n = sizeD2M(d);
	buf = malloc(n);
	if(buf == nil)
		return -1;
	convD2M(d, buf, n);
	r = name ? p9web_wstat(name, strlen(name), buf, n) : p9web_fwstat(fd, buf, n);
	free(buf);
	if(r < 0){
		kerr(-r);
		return -1;
	}
	return 0;
}

int dirwstat(char *name, Dir *d) { return put(d, name, -1); }
int dirfwstat(int fd, Dir *d) { return put(d, nil, fd); }

/*
 * dirread and dirreadall are Plan 9's (dirread.c): a read of the directory gives whole stats,
 * which dirpackage converts. The kernel's stats carry 8 bytes more (abi.ts); convM2D takes
 * the fields it knows.
 */
static long
dirpackage(uchar *buf, long ts, Dir **d)
{
	char *s;
	long ss, i, n, nn, m;

	*d = nil;
	if(ts <= 0)
		return 0;
	ss = 0;
	n = 0;
	for(i = 0; i < ts; i += m){
		m = BIT16SZ + GBIT16(&buf[i]);
		if(m < STATFIXLEN || i + m > ts)
			break;
		ss += m;
		n++;
	}
	if(i != ts){
		werrstr("bad directory format");
		return -1;
	}
	*d = malloc(n * sizeof(Dir) + ss);
	if(*d == nil)
		return -1;
	s = (char*)&(*d)[n];
	nn = 0;
	for(i = 0; i < ts; i += m){
		m = BIT16SZ + GBIT16(&buf[i]);
		if(nn >= n || convM2D(&buf[i], m, *d + nn, s) == 0){
			free(*d);
			*d = nil;
			werrstr("bad directory entry");
			return -1;
		}
		nn++;
		s += m;
	}
	return nn;
}

long
dirread(int fd, Dir **d)
{
	uchar *buf;
	long ts;

	buf = malloc(DIRMAX);
	if(buf == nil)
		return -1;
	ts = read(fd, buf, DIRMAX);
	if(ts >= 0)
		ts = dirpackage(buf, ts, d);
	free(buf);
	return ts;
}

long
dirreadall(int fd, Dir **d)
{
	uchar *buf, *nbuf;
	long n, ts;

	buf = nil;
	ts = 0;
	for(;;){
		nbuf = realloc(buf, ts + DIRMAX);
		if(nbuf == nil){
			free(buf);
			return -1;
		}
		buf = nbuf;
		n = read(fd, buf + ts, DIRMAX);
		if(n <= 0)
			break;
		ts += n;
	}
	if(ts >= 0)
		ts = dirpackage(buf, ts, d);
	free(buf);
	if(ts == 0 && n < 0)
		return -1;
	return ts;
}
