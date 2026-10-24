#include <stdbool.h>
#include "diff.h"

/*
 * Fall back to diffreg_stone (standard Berkeley diff algorithm).
 */
bool
can_libdiff(int flags)
{
	(void)flags;
	return false;
}

int
diffreg_new(char *file1, char *file2, int flags, int capsicum)
{
	(void)file1;
	(void)file2;
	(void)flags;
	(void)capsicum;
	return 0;
}
