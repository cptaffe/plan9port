#include <u.h>
#include <libc.h>
#include <sys/time.h>

int touch(int, char *);
ulong now;
int tflag;

void
usage(void)
{
	fprint(2, "usage: touch [-c] [-t time] files\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int nocreate = 0;
	int status = 0;

	now = time(0);
	ARGBEGIN{
	case 't':
		tflag = 1;
		now = strtoul(EARGF(usage()), 0, 0);
		break;
	case 'c':
		nocreate = 1;
		break;
	default:
		usage();
	}ARGEND

	if(!*argv)
		usage();
	while(*argv)
		status += touch(nocreate, *argv++);
	if(status)
		exits("touch");
	exits(0);
}

int
touch(int nocreate, char *name)
{
	Dir stbuff;
	int fd;

	/*
	 * p9web: with no -t, set the time to now at full precision, as the
	 * UNIX touch does. Whole seconds made a file older than the file it was
	 * touched after, and mk then rebuilt it.
	 */
	if(!tflag && access(name, 0) >= 0 && utimes(name, 0) >= 0)
		return 0;
	nulldir(&stbuff);
	stbuff.mtime = now;
	if(dirwstat(name, &stbuff) >= 0)
		return 0;
	if(nocreate){
		fprint(2, "touch: %s: cannot wstat: %r\n", name);
		return 1;
	}
	if((fd = create(name, OWRITE, 0666)) < 0) {
		fprint(2, "touch: %s: cannot create: %r\n", name);
		return 1;
	}
	if(tflag && dirfwstat(fd, &stbuff) < 0){
		fprint(2, "touch: %s: cannot wstat: %r\n", name);
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}
