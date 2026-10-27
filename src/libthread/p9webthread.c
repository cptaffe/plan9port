/*
 * p9web: libthread's system part on the runtime's fibers (k/src/proc/fibers.ts), in place of
 * pthread.c and daemonize.c. Each pthread of plan9port's design is a fiber: a coroutine in
 * this instance with its own stack and its own kernel process (rfork RFPROC|RFMEM), which
 * switches only in a call that waits and in the imports below. So a Lock is a word, and a
 * held one is waited for by yielding; a _Procrendez is a fiber's block and wake.
 */
#include "threadimpl.h"
#include "p9web.h"

#undef exits
#undef _exits

/* Start a fiber at p9web_fiber_main(fn, arg) on stk[nstk], with abuf[nabuf] for asyncify, in a
 * process of its own (own: a new proc) or this fiber's (a thread of this fiber's proc). */
P9IMPORT("fiber_new") int p9web_fiber_new(void (*fn)(void*), void *arg, void *stk, int nstk, void *abuf, int nabuf, int own);
P9IMPORT("fiber_self") int p9web_fiber_self(void);
P9IMPORT("fiber_getdata") void *p9web_fiber_getdata(void);
P9IMPORT("fiber_setdata") void p9web_fiber_setdata(void*);
/* Sleep until a fiber_wake of this fiber (one that came first counts). */
P9IMPORT("fiber_block") void p9web_fiber_block(void);
P9IMPORT("fiber_wake") void p9web_fiber_wake(int);
/* Let the other fibers run. */
P9IMPORT("fiber_yield") void p9web_fiber_yield(void);
/* End this fiber and its process. */
P9IMPORT("fiber_exit") void p9web_fiber_exit(void);

enum
{
	MINSTACK = 64*1024,	/* the C stack: only a function's arrays and the locals whose address it takes */
	ABUF = 64*1024,	/* what asyncify saves of a stack (Emscripten's default of 4 KB is too small) */
};

/* A fiber's memory. An ended fiber's is freed by the next new fiber, as no fiber can free
 * the stack it runs on. */
typedef struct Fibermem Fibermem;
struct Fibermem
{
	int id;
	void *stk;
	void *abuf;
	Fibermem *next;
};
static Fibermem *live, *dead;

__attribute__((export_name("p9web_fiber_main"))) void
p9web_fiber_main(void (*fn)(void*), void *arg)
{
	fn(arg);
	_threadpexit();
}

static void
newfiber(void (*fn)(void*), void *arg, uint stack, int own)
{
	Fibermem *m;
	void *stk, *abuf;
	int r;

	while((m = dead) != nil){
		dead = m->next;
		free(m->stk);
		free(m->abuf);
		free(m);
	}
	if(stack < MINSTACK)
		stack = MINSTACK;
	m = malloc(sizeof *m);
	stk = malloc(stack);
	abuf = malloc(ABUF);
	if(m == nil || stk == nil || abuf == nil)
		sysfatal("newfiber malloc: %r");
	m->stk = stk;
	m->abuf = abuf;
	if((r = p9web_fiber_new(fn, arg, stk, stack, abuf, ABUF, own)) < 0){
		errno = -r;
		sysfatal("newfiber: %r");
	}
	m->id = r;
	m->next = live;
	live = m;
}

int
_threadlock(Lock *lk, int block, ulong pc)
{
	USED(pc);
	while(lk->held){
		if(!block)
			return 0;
		p9web_fiber_yield();
	}
	lk->held = 1;
	return 1;
}

void
_threadunlock(Lock *lk, ulong pc)
{
	USED(pc);
	lk->held = 0;
}

/* note: _procsleep can have spurious wakeups, like pthread_cond_wait */
void
_procsleep(_Procrendez *r)
{
	/* r is protected by r->l, which we hold; no other fiber runs before the block */
	r->asleep = 1;
	r->fiber = p9web_fiber_self();
	_threadunlock(r->l, 0);
	p9web_fiber_block();
	_threadlock(r->l, 1, 0);
	r->asleep = 0;
}

void
_procwakeup(_Procrendez *r)
{
	if(r->asleep){
		r->asleep = 0;
		p9web_fiber_wake(r->fiber);
	}
}

void
_procwakeupandunlock(_Procrendez *r)
{
	_procwakeup(r);
	unlock(r->l);
}

static void
startprocfn(void *v)
{
	void **a;
	void (*fn)(Proc*);
	Proc *p;

	a = (void**)v;
	fn = (void(*)(Proc*))a[0];
	p = a[1];
	free(a);
	p->osprocid = p9web_fiber_self();
	(*fn)(p);
}

static void
startpthreadfn(void *v)
{
	void **a;
	Proc *p;
	_Thread *t;

	a = (void**)v;
	p = a[0];
	t = a[1];
	free(a);
	t->osprocid = p9web_fiber_self();
	_threadpthreadmain(p, t);
}

void
_procstart(Proc *p, void (*fn)(Proc*))
{
	void **a;

	a = malloc(2*sizeof a[0]);
	if(a == nil)
		sysfatal("_procstart malloc: %r");
	a[0] = (void*)fn;
	a[1] = p;
	newfiber(startprocfn, a, p->runqueue.head ? p->runqueue.head->stksize : 0, 1);
}

void
_threadpthreadstart(Proc *p, _Thread *t)
{
	void **a;

	a = malloc(2*sizeof a[0]);
	if(a == nil)
		sysfatal("_pthreadstart malloc: %r");
	a[0] = p;
	a[1] = t;
	/* t's proc is this fiber's: a proc's threads are made by its own threads (threadcreate) */
	newfiber(startpthreadfn, a, t->stksize, 0);
}

Proc*
_threadproc(void)
{
	return p9web_fiber_getdata();
}

void
_threadsetproc(Proc *p)
{
	p9web_fiber_setdata(p);
}

void
_pthreadinit(void)
{
}

void
threadexitsall(char *msg)
{
	exits(msg);
}

void
_threadpexit(void)
{
	Fibermem **l, *m;
	int self;

	self = p9web_fiber_self();
	for(l = &live; (m = *l) != nil; l = &m->next)
		if(m->id == self){
			*l = m->next;
			m->next = dead;
			dead = m;
			break;
		}
	p9web_fiber_exit();
}

/* _threadpark blocks this fiber for good, with no call: the proc's other threads keep its slot. */
void
_threadpark(void)
{
	for(;;)
		p9web_fiber_block();
}

/* A libthread program does not go into the background on p9web (bg.c says it may not). */
void
_threadsetupdaemonize(void)
{
}

void
_threaddaemonize(void)
{
}
