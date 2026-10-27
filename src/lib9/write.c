#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

long
p9write(int f, void *av, long n)
{
	char *a;
	long m, t;

	a = av;
	if(n == 0)	/* p9web: a write of nothing goes to the file, as on Plan 9 (Acme's data deletes) */
		return write(f, a, 0);
	t = 0;
	while(t < n){
		m = write(f, a+t, n-t);
		if(m <= 0){
			if(t == 0)
				return m;
			break;
		}
		t += m;
	}
	return t;
}
