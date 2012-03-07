/* vi: set sw=4 ts=4:
 *
 * chgrp.c - Change group ownership
 *
 * Copyright 2012 Georgi Chorbadzhiyski <georgi@unixsol.org>
 *
 * See http://pubs.opengroup.org/onlinepubs/009695399/utilities/chgrp.html
 *
 * TODO: Add support for -h
 * TODO: Add support for -H
 * TODO: Add support for -L
 * TODO: Add support for -P

USE_CHGRP(NEWTOY(chgrp, "<2Rfv", TOYFLAG_BIN))

config CHGRP
	bool "chgrp"
	default y
	help
	  usage: chgrp [-R] [-f] [-v] group file...
	  Change group ownership of one or more files.

	  -R	recurse into subdirectories.
	  -f	suppress most error messages.
	  -v	verbose output.
*/

#include "toys.h"

#define FLAG_R 4
#define FLAG_f 2
#define FLAG_v 1

DEFINE_GLOBALS(
	gid_t group;
	char *group_name;
)

#define TT this.chgrp

static int do_chgrp(char *path) {
	int ret = chown(path, -1, TT.group);
	if (toys.optflags & FLAG_v)
		xprintf("chgrp(%s, %s)\n", TT.group_name, path);
	if (ret == -1 && !(toys.optflags & FLAG_f))
		perror_msg("changing group of '%s' to '%s'", path, TT.group_name);
	toys.exitval |= ret;
	return ret;
}

void chgrp_main(void)
{
	char **s;
	struct group *group;

	TT.group_name = *toys.optargs;
	group = getgrnam(TT.group_name);
	if (!group) {
		error_msg("invalid group '%s'", TT.group_name);
		toys.exitval = 1;
		return;
	}
	TT.group = group->gr_gid;

	for (s=toys.optargs + 1; *s; s++) {
		if (toys.optflags & FLAG_R) {
			dirtree_for_each(*s, do_chgrp);
		} else {
			do_chgrp(*s);
		}
	}
}
