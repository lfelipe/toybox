/* chgrp.c - Change user and group ownership
 *
 * Copyright 2012 Georgi Chorbadzhiyski <georgi@unixsol.org>
 *
 * See http://opengroup.org/onlinepubs/9699919799/utilities/chown.html
 * See http://opengroup.org/onlinepubs/9699919799/utilities/chgrp.html

USE_CHGRP(NEWTOY(chgrp, "<2hPLHRfv[-HLP]", TOYFLAG_BIN))
USE_CHOWN(OLDTOY(chown, chgrp, TOYFLAG_BIN))

config CHGRP
  bool "chgrp"
  default y
  help
    usage: chgrp/chown [-RHLP] [-fvh] group file...

    Change group of one or more files.

    -f	suppress most error messages.
    -h	change symlinks instead of what they point to
    -R	recurse into subdirectories (implies -h).
    -H	with -R change target of symlink, follow command line symlinks
    -L	with -R change target of symlink, follow all symlinks
    -P	with -R change symlink, do not follow symlinks (default)
    -v	verbose output.

config CHOWN
  bool "chown"
  default y
  help
    see: chgrp
*/

#define FOR_chgrp
#define FORCE_FLAGS
#include "toys.h"

GLOBALS(
  uid_t owner;
  gid_t group;
  char *owner_name, *group_name;
  int symfollow;
)

static int do_chgrp(struct dirtree *node)
{
  int fd, ret, flags = toys.optflags;

  // Depth first search
  if (!dirtree_notdotdot(node)) return 0;
  if ((flags & FLAG_R) && !node->again && S_ISDIR(node->st.st_mode))
    return DIRTREE_COMEAGAIN|((flags&FLAG_L) ? DIRTREE_SYMFOLLOW : 0);

  fd = dirtree_parentfd(node);
  ret = fchownat(fd, node->name, TT.owner, TT.group,
    (flags&(FLAG_L|FLAG_H)) || !(flags&(FLAG_h|FLAG_R))
      ? 0 : AT_SYMLINK_NOFOLLOW);

  if (ret || (flags & FLAG_v)) {
    char *path = dirtree_path(node, 0);
    if (flags & FLAG_v)
      xprintf("%s %s%s%s %s\n", toys.which->name,
        TT.owner_name ? TT.owner_name : "",
        toys.which->name[2]=='o' && TT.group_name ? ":" : "",
        TT.group_name ? TT.group_name : "", path);
    if (ret == -1 && !(toys.optflags & FLAG_f))
      perror_msg("'%s' to '%s:%s'", path, TT.owner_name, TT.group_name);
    free(path);
  }
  toys.exitval |= ret;

  return 0;
}

void chgrp_main(void)
{
  int ischown = toys.which->name[2] == 'o', hl = toys.optflags&(FLAG_H|FLAG_L);
  char **s, *own;

  TT.owner = TT.group = -1;

  // Distinguish chown from chgrp
  if (ischown) {
    char *grp;
    struct passwd *p;

    own = xstrdup(*toys.optargs);
    if ((grp = strchr(own, ':')) || (grp = strchr(own, '.'))) {
      *(grp++) = 0;
      TT.group_name = grp;
    }
    if (*own) {
      TT.owner_name = own;
      p = getpwnam(own);
      // TODO: trailing garbage?
      if (!p && isdigit(*own)) p=getpwuid(atoi(own));
      if (!p) error_exit("no user '%s'", own);
      TT.owner = p->pw_uid;
    }
  } else TT.group_name = *toys.optargs;

  if (TT.group_name && *TT.group_name) {
    struct group *g;
    g = getgrnam(TT.group_name);
    if (!g) g=getgrgid(atoi(TT.group_name));
    if (!g) error_exit("no group '%s'", TT.group_name);
    TT.group = g->gr_gid;
  }

  for (s=toys.optargs+1; *s; s++) {
    struct dirtree *new = dirtree_add_node(0, *s, hl);
    if (new) dirtree_handle_callback(new, do_chgrp);
    else toys.exitval = 1;
  }

  if (CFG_TOYBOX_FREE && ischown) free(own);
}

void chown_main()
{
  chgrp_main();
}
