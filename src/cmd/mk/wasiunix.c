/*
 * mk's operating-system interface on p9web, in place of unix.c.
 * unix.c forks; here each job is a p9web spawn of the shell, with pipes for
 * its standard input (the recipe) and, for a backquote or <|, its output.
 * Jobs run at the same time (NPROC), and waitfor blocks in p9web.wait.
 */
#define NOPLAN9DEFINES
#include	"mk.h"
#include	<sys/wait.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	"p9web.h"

char	*shell = "/bin/sh";
char	*shellname = "sh";

extern char **environ;

/* The environment of the next job: exportenv builds it, spawn passes it. */
static char **jobenv;

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

void
exportenv(Envy *e, Shell *sh)
{
	int w, n;
	char **p;
	Envy *e1;

	if(jobenv){
		for(p = jobenv; *p; p++)
			free(*p);
		free(jobenv);
	}
	n = 0;
	for(e1 = e; e1->name; e1++)
		n++;
	p = Malloc((n+1)*sizeof(char*));
	w = 0;
	for(; e->name; e++) {
		if(sh == &rcshell && (e->values == 0 || e->values->s == 0 || e->values->s[0] == 0))
			continue; /* do not write empty string for empty list */
		if(e->values)
			p[w++] = smprint("%s=%s", e->name, wtos(e->values, sh->iws));
		else
			p[w++] = smprint("%s=", e->name);
	}
	p[w] = 0;
	jobenv = p;
}

int
waitfor(char *msg)
{
	int status;
	int pid;

	*msg = 0;
	pid = wait(&status);
	if(pid > 0) {
		if(status&0x7f)
			snprint(msg, ERRMAX, "signal %d", status&0x7f);
		else if(status&0xff00)
			snprint(msg, ERRMAX, "exit(%d)", (status>>8)&0xff);
	}
	return pid;
}

void
expunge(int pid, char *msg)
{
	if(strcmp(msg, "interrupt"))
		kill(pid, SIGINT);
	else
		kill(pid, SIGHUP);
}

int mypid;

int
shargv(Word *cmd, int extra, char ***pargv)
{
	char **argv;
	int i, n;
	Word *w;

	n = 0;
	for(w=cmd; w; w=w->next)
		n++;

	argv = Malloc((n+extra+1)*sizeof(argv[0]));
	i = 0;
	for(w=cmd; w; w=w->next)
		argv[i++] = w->s;
	argv[n] = 0;
	*pargv = argv;
	return n;
}

/* Start the shell with fds in, out, 2; the environment from exportenv if e. */
static int
spawnsh(char **argv, Envy *e, Shell *sh, int in, int out)
{
	int fdmap[3], pid;

	if(e)
		exportenv(e, sh);
	fdmap[0] = in;
	fdmap[1] = out;
	fdmap[2] = 2;
	pid = p9spawn(argv[0], argv, e ? jobenv : environ, fdmap, 3);
	if(pid < 0){
		mkperror(argv[0]);
		Exit();
	}
	return pid;
}

int
execsh(char *args, char *cmd, Bufblock *buf, Envy *e, Shell *sh, Word *shellcmd)
{
	char *p, **argv;
	int tot, n, pid, in[2], out[2];

	if(pipe(in) < 0 || (buf && pipe(out) < 0)){
		mkperror("pipe");
		Exit();
	}
	n = shargv(shellcmd, 1, &argv);
	argv[n++] = args;
	argv[n] = 0;
	pid = spawnsh(argv, e, sh, in[0], buf ? out[1] : 1);
	free(argv);
	close(in[0]);
	if(buf)
		close(out[1]);
	mypid = getpid();
	if(DEBUG(D_EXEC))
		fprint(1, "starting: %s\n", cmd);
	/* The shell reads the recipe as it runs; a backquote's output is short. */
	p = cmd+strlen(cmd);
	while(cmd < p){
		n = write(in[1], cmd, p-cmd);
		if(n < 0)
			break;
		cmd += n;
	}
	close(in[1]);
	if(buf){
		tot = 0;
		for(;;){
			if (buf->current >= buf->end)
				growbuf(buf);
			n = read(out[0], buf->current, buf->end-buf->current);
			if(n <= 0)
				break;
			buf->current += n;
			tot += n;
		}
		if (tot && buf->current[-1] == '\n')
			buf->current--;
		close(out[0]);
	}
	return pid;
}

int
pipecmd(char *cmd, Envy *e, int *fd, Shell *sh, Word *shellcmd)
{
	int pid, pfd[2];
	int n;
	char **argv;

	if(DEBUG(D_EXEC))
		fprint(1, "pipecmd='%s'\n", cmd);/**/

	if(fd && pipe(pfd) < 0){
		mkperror("pipe");
		Exit();
	}
	n = shargv(shellcmd, 2, &argv);
	argv[n++] = "-c";
	argv[n++] = cmd;
	argv[n] = 0;
	pid = spawnsh(argv, e, sh, 0, fd ? pfd[1] : 1);
	free(argv);
	if(fd){
		close(pfd[1]);
		*fd = pfd[0];
	}
	return pid;
}

void
Exit(void)
{
	while(wait(0) >= 0)
		;
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
		snprint(buf, sizeof buf, "/tmp/mkarg%d.%d", getpid(), n++);
	while((fd = open(buf, O_RDWR|O_CREAT|O_EXCL, 0600)) < 0 && n < 10000);
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

	*to = match->s.sp;		/* stem0 matches complete target */
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

vlong
mkmtime(char *name)
{
	struct stat st;

	if(stat(name, &st) < 0)
		return 0;
	return st.st_mtim.tv_sec*1000000000LL + st.st_mtim.tv_nsec;
}
