/*
 * Plan 9's notes on p9web's kernel, in place of lib9's notify.c (POSIX signals): notify tells
 * the kernel that the program has a handler, and the runtime calls p9web_note for each note
 * (k/src/proc/worker.ts), at the end of the call the note interrupted, or of the next. The
 * handler's noted jumps back to p9web_note, whose answer says what the runtime does: go on
 * (NCONT), or end the program with the note (NDFLT, or a handler that returns).
 */
#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>
#include "p9web.h"

typedef struct Jmp Jmp;
struct Jmp
{
	p9jmp_buf b;
};

static Jmp onejmp;

static Jmp*
getonejmp(void)
{
	return &onejmp;
}

Jmp *(*_notejmpbuf)(void) = getonejmp;	/* libthread puts its own */

static void (*notifyf)(void*, char*);
static char notebuf[ERRMAX];

int
notify(void (*f)(void*, char*))
{
	notifyf = f;
	return p9web_notify(f != nil) < 0 ? -1 : 0;
}

int
noted(int v)
{
	p9longjmp((*_notejmpbuf)()->b, v == NCONT ? 2 : 1);
	abort();
	return 0;
}

__attribute__((export_name("p9web_note_buf"))) char*
p9web_note_buf(void)
{
	return notebuf;
}

/* The note is in notebuf: 1 if the handler said NCONT. */
__attribute__((export_name("p9web_note"))) int
p9web_note(void)
{
	switch(p9setjmp((*_notejmpbuf)()->b)){
	case 0:
		if(notifyf)
			(*notifyf)(nil, notebuf);
		return 0;
	case 2:
		return 1;
	default:
		return 0;
	}
}

/* Every note goes to the handler: there are no signals to turn on or off. */
int notifyon(char *s) { USED(s); return 0; }
int notifyoff(char *s) { USED(s); return 0; }
int notifyignore(char *s) { USED(s); return 0; }
int notedisable(char *s) { USED(s); return 0; }
int noteenable(char *s) { USED(s); return 0; }
