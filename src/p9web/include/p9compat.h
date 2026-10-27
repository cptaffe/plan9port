/* Included before every file: WASI has setjmp (wasm exceptions), but no signal masks. */
#include <setjmp.h>
#ifndef sigsetjmp
typedef jmp_buf sigjmp_buf;
#define sigsetjmp(b, s) setjmp(b)
#define siglongjmp(b, v) longjmp(b, v)
#endif
