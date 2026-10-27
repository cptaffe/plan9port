/*
 * rc's process layer on p9web: ForkExecute spawns a command with rc's
 * descriptors, the first three from its redirections (-1: none). rc forks (havefork.c) for a pipeline element, a
 * backquote, a subshell and <{}, but starts a command with spawn, not with a
 * fork and an exec (D93).
 */
#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"
#include "../p9web/p9web.h"

extern char **mkenv(void);
extern void addwaitpid(int);

int
ForkExecute(char *file, char **argv, int sin, int sout, int serr)
{
	int fdmap[3], pid;
	char **env;

	fdmap[0] = sin;
	fdmap[1] = sout;
	fdmap[2] = serr;
	env = mkenv();
	/* As a fork and an exec: the command has every descriptor rc has, as <{} needs. */
	pid = p9spawnfd(file, argv, env, fdmap, 3, RFFDG);
	efree((char*)env);
	/* havefork.c records each child for Waitfor; haventfork.c does not. */
	if(pid > 0)
		addwaitpid(pid);
	return pid;
}

/*
 *  start a process running the cmd on the stack and return its pid.
 */
int
execforkexec(void)
{
	char **argv;
	char file[1024];
	int nc;
	word *path;
	int pid;

	if(runq->argv->words==0)
		return -1;
	argv = mkargv(runq->argv->words);

	for(path = searchpath(runq->argv->words->word);path;path = path->next){
		nc = strlen(path->word);
		if(nc<sizeof(file)){
			strcpy(file, path->word);
			if(file[0]){
				strcat(file, "/");
				nc++;
			}
			if(nc+strlen(argv[1])<sizeof(file)){
				strcat(file, argv[1]);
				pid = ForkExecute(file, argv+1, mapfd(0), mapfd(1), mapfd(2));
				if(pid >= 0){
					free(argv);
					return pid;
				}
			}
		}
	}
	/* No program on the path: say so on the command's own fd 2, as the child of a
	 * fork whose exec fails does (Execute), and let the caller fail the command. */
	rerrstr(file, sizeof file);
	fprint(mapfd(2), "%s: %s\n", argv[1], file);
	free(argv);
	return -2;
}

long
readnb(int fd, char *buf, long cnt)
{
	return read(fd, buf, cnt);
}

void execulimit(void) { Xreturn(); }
void execumask(void) { Xreturn(); }
