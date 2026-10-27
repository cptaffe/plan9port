#include <u.h>
#include <libc.h>
#ifdef __wasm__
#include "p9web.h"
#endif

void
_exits(char *s)
{
	if(s == 0 || *s == 0)
		_exit(0);
#ifdef __wasm__
	p9web_exitmsg(s, strlen(s));
#endif
	_exit(exitcode(s));
}
