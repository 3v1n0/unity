noinst_LTLIBRARIES = \
  libunity-quicklauncher.la

# unity-quicklauncher static lib
libunity_quicklauncher_la_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_quicklauncher_la_VALAFLAGS = \
  -H unity-quicklauncher.h -d . \
  --library unity-quicklauncher \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  $(MAINTAINER_VALAFLAGS)

libunity_quicklauncher_la_LIBADD = \
  $(BASE_LIBS)

libunity_quicklauncher_la_SOURCES = \
  quicklauncher.vala

CLEANFILES = *.stamp
