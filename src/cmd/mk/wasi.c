#define NOPLAN9DEFINES
#include	"mk.h"
#include	<errno.h>
#include	<sys/stat.h>
#include	<sys/time.h>

/*
 * WASI has no processes, so the host runs each command.
 * run starts argv in environment env (lists of NUL-terminated
 * strings; an empty env means mk's own) with standard input in,
 * waits for it, and returns its exit status.  If capture is set,
 * outlen and out give the command's standard output.
 */
#define HOST(f)	__attribute__((import_module("mkhost"), import_name(f)))

HOST("run")	int	hostrun(char*, int, char*, int, char*, int, int);
HOST("outlen")	int	hostoutlen(void);
HOST("out")	void	hostout(char*);

typedef struct Child Child;
struct Child
{
	int	pid;
	int	status;
	Child	*next;
};

static Child *children;
static Child **echildren = &children;
static int npid;

extern char **environ;

static void
mkperror(char *s)
{
	fprint(2, "%s: %r\n", s);
}

void
readenv(void)
{
	char **p, *s;
	Word *w;

	for(p = environ; *p; p++){
		s = strchr(*p, '=');
		if(s){
			*s = 0;
			w = newword(s+1);
		} else
			w = newword("");
		if (symlook(*p, S_INTERNAL, 0))
			continue;
		s = strdup(*p);
		setvar(s, (void *)w);
		symlook(s, S_EXPORTED, (void*)"")->u.ptr = "";
	}
}

static void
addstr(Bufblock *b, char *s)
{
	bufcpy(b, s, strlen(s));
	insert(b, 0);
}

static void
addenv(Bufblock *b, Envy *e, Shell *sh)
{
	char *s;

	for(; e->name; e++) {
		if(sh == &rcshell && (e->values == 0 || e->values->s == 0 || e->values->s[0] == 0))
			continue;
		bufcpy(b, e->name, strlen(e->name));
		insert(b, '=');
		if(e->values){
			s = wtos(e->values, sh->iws);
			bufcpy(b, s, strlen(s));
			free(s);
		}
		insert(b, 0);
	}
}

static int
runcmd(Word *cmd, char **args, int nargs, char *in, Envy *e, Shell *sh, int capture)
{
	Bufblock *argv, *env;
	Child *c;
	int i;

	argv = newbuf();
	for(; cmd; cmd = cmd->next)
		addstr(argv, cmd->s);
	for(i = 0; i < nargs; i++)
		addstr(argv, args[i]);
	env = newbuf();
	if(e)
		addenv(env, e, sh);
	c = Malloc(sizeof(Child));
	c->pid = ++npid;
	c->status = hostrun(argv->start, argv->current-argv->start,
		env->start, env->current-env->start,
		in, in ? strlen(in) : 0, capture);
	c->next = 0;
	*echildren = c;
	echildren = &c->next;
	freebuf(argv);
	freebuf(env);
	return c->pid;
}

static char*
output(int *n)
{
	char *p;

	*n = hostoutlen();
	p = Malloc(*n+1);
	hostout(p);
	return p;
}

int
waitfor(char *msg)
{
	Child *c;
	int pid;

	*msg = 0;
	c = children;
	if(c == 0)
		return -1;
	children = c->next;
	if(children == 0)
		echildren = &children;
	pid = c->pid;
	if(c->status)
		snprint(msg, ERRMAX, "exit(%d)", c->status);
	free(c);
	return pid;
}

void
expunge(int pid, char *msg)
{
	USED(pid);
	USED(msg);
}

int
execsh(char *args, char *cmd, Bufblock *buf, Envy *e, Shell *sh, Word *shellcmd)
{
	char *p;
	int n, pid;

	if(DEBUG(D_EXEC))
		fprint(1, "starting: %s\n", cmd);
	pid = runcmd(shellcmd, &args, args != 0, cmd, e, sh, buf != 0);
	if(buf){
		p = output(&n);
		bufcpy(buf, p, n);
		free(p);
		if(n && buf->current[-1] == '\n')
			buf->current--;
	}
	return pid;
}

int
pipecmd(char *cmd, Envy *e, int *fd, Shell *sh, Word *shellcmd)
{
	char *p, *args[2], *name;
	int n, pid;

	if(DEBUG(D_EXEC))
		fprint(1, "pipecmd='%s'\n", cmd);

	args[0] = "-c";
	args[1] = cmd;
	pid = runcmd(shellcmd, args, 2, 0, e, sh, fd != 0);
	if(fd){
		p = output(&n);
		name = maketmp(fd);
		if(name == 0){
			mkperror("mk pipecmd");
			Exit();
		}
		unlink(name);
		write(*fd, p, n);
		lseek(*fd, 0, 0);
		free(p);
	}
	return pid;
}

void
Exit(void)
{
	exits("error");
}

void
catchnotes(void)
{
}

char*
maketmp(int *pfd)
{
	static char buf[100];
	static int n;
	int fd;

	do
		snprint(buf, sizeof buf, "/tmp/mkarg%d", n++);
	while((fd = open(buf, O_RDWR|O_CREAT|O_EXCL, 0600)) < 0 && errno == EEXIST);
	if(fd < 0)
		return 0;
	*pfd = fd;
	return buf;
}

int
chgtime(char *name)
{
	if(access(name, 0) >= 0)
		return utimes(name, 0);
	return close(creat(name, 0666));
}

void
rcopy(char **to, Resub *match, int n)
{
	int c;
	char *p;

	*to = match->s.sp;
	for(to++, match++; --n > 0; to++, match++){
		if(match->s.sp && match->e.ep){
			p = match->e.ep;
			c = *p;
			*p = 0;
			*to = strdup(match->s.sp);
			*p = c;
		}
		else
			*to = 0;
	}
}

unsigned long
mkmtime(char *name)
{
	struct stat st;

	if(stat(name, &st) < 0)
		return 0;

	return st.st_mtime;
}
