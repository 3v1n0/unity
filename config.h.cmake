#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine PREFIXDIR "@PREFIXDIR@"
#cmakedefine DATADIR "@DATADIR@"
#cmakedefine PKGDATADIR "@PKGDATADIR@"
#cmakedefine LOCALE_DIR "@LOCALE_DIR@"
#cmakedefine VERSION "@VERSION@"
#cmakedefine BUILDDIR "@BUILDDIR@"
#cmakedefine TESTVALADIR "@TESTVALADIR@"
#cmakedefine TESTDATADIR "@TESTDIRDIR@"

#define STRLOC g_print ("%s\n", G_STRLOC);

#endif // CONFIG_H
