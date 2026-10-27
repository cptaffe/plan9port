/* lib9's main for a program whose p9main returns int (int, as rc and mk declare it; wasm checks the type). */
#include <u.h>
#define NOPLAN9DEFINES
#include <libc.h>

extern int p9main(int, char**);

int
main(int argc, char **argv)
{
	p9main(argc, argv);
	exits(0);
	return 0;
}
