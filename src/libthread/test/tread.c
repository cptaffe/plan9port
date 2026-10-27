/*
 * p9web: a thread computes while another proc waits in read. The reader's proc waits in the
 * kernel; the main thread's sleep lets it get there first, then the main thread computes and
 * writes what the reader reads. If a proc's wait held the others, this would not end.
 */
#include <u.h>
#include <libc.h>
#include <thread.h>

int p[2];
Channel *done;

void
reader(void *v)
{
	char buf[64];
	int n;

	USED(v);
	n = read(p[0], buf, sizeof buf - 1);
	buf[n > 0 ? n : 0] = 0;
	sendp(done, strdup(buf));
}

void
threadmain(int argc, char **argv)
{
	vlong i, sum;

	USED(argc);
	USED(argv);
	if(pipe(p) < 0)
		sysfatal("pipe: %r");
	done = chancreate(sizeof(char*), 0);
	proccreate(reader, nil, 32768);
	sleep(10);
	sum = 0;
	for(i = 0; i < 10000000; i++)
		sum += i;
	print("computed %lld\n", sum);
	fprint(p[1], "hello %lld", sum % 1000);
	print("read %s\n", (char*)recvp(done));
	threadexitsall(nil);
}
