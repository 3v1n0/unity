SUBDIRS = \
  quicklauncher

noinst_LTLIBRARIES = \
  libunity-static.la

bin_PROGRAMS = \
  unity

# unity static lib 
libunity_static_la_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_static_la_VALAFLAGS = \
  -H unity-static.h -d . \
  --library unity-static \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  $(MAINTAINER_VALAFLAGS)

libunity_static_la_LIBADD = \
  $(BASE_LIBS)

libunity_static_la_SOURCES = \
  unity.vala

# unity binary
unity_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

unity_VALAFLAGS = \
  --vapidir=./ \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg unity-static \
  $(MAINTAINER_VALAFLAGS)

unity_LDADD = \
  libunity-static.la \
  $(BASE_LIBS)

unity_SOURCES = \
  main.vala

CLEANFILES = *.stamp
