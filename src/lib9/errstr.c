/*
 * We assume there's only one error buffer for the whole system.
 * If you use ffork, you need to provide a _syserrstr.  Since most
 * people will use libthread (which provides a _syserrstr), this is
 * okay.
 */

#include <u.h>
#include <errno.h>
#include <string.h>
#include <libc.h>

enum
{
	EPLAN9 = 0x19283745
};

#ifdef __wasm__
#include "p9web.h"

/* p9web: the kernel's text for errno, if errno is the kernel's last error's: Plan 9's strings,
 * not strerror's. */
static int
kernelerr(char *buf, uint n)
{
	int e;

	e = p9web_errstr(buf, n);
	return e != 0 && e == errno;
}
#else
static int kernelerr(char *buf, uint n) { USED(buf); USED(n); return 0; }
#endif

char *(*_syserrstr)(void);
static char xsyserr[ERRMAX];
static char*
getsyserr(void)
{
	char *s;

	s = nil;
	if(_syserrstr)
		s = (*_syserrstr)();
	if(s == nil)
		s = xsyserr;
	return s;
}

int
errstr(char *err, uint n)
{
	char tmp[ERRMAX];
	char *syserr;

	strecpy(tmp, tmp+ERRMAX, err);
	rerrstr(err, n);
	syserr = getsyserr();
	strecpy(syserr, syserr+ERRMAX, tmp);
	errno = EPLAN9;
	return 0;
}

void
rerrstr(char *err, uint n)
{
	char *syserr;

	syserr = getsyserr();
	if(errno == EINTR)
		strcpy(syserr, "interrupted");
	else if(errno != EPLAN9 && !kernelerr(syserr, ERRMAX))
		strcpy(syserr, strerror(errno));
	strecpy(err, err+n, syserr);
}

/* replaces __errfmt in libfmt */

int
__errfmt(Fmt *f)
{
	char buf[ERRMAX];

	if(errno == EPLAN9)
		return fmtstrcpy(f, getsyserr());
	if(kernelerr(buf, sizeof buf))
		return fmtstrcpy(f, buf);
	return fmtstrcpy(f, strerror(errno));
}

void
werrstr(char *fmt, ...)
{
	va_list arg;
	char buf[ERRMAX];

	va_start(arg, fmt);
	vseprint(buf, buf+ERRMAX, fmt, arg);
	va_end(arg);
	errstr(buf, ERRMAX);
}
