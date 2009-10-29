noinst_LTLIBRARIES = \
  libunity.la

# unity-quicklauncher static lib
libunity_la_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_la_VALAFLAGS = \
  -H unity.h -d . \
  --library unity \
  --vapidir=$(top_builddir)/vapi/ \
  --pkg clutter-1.0 \
  --pkg clutk-0.3 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  $(MAINTAINER_VALAFLAGS)

libunity_la_LIBADD = \
  $(BASE_LIBS)

libunity_la_SOURCES = \
  shell.vala

CLEANFILES = \
  *.stamp
