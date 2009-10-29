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
  --vapidir=$(top_builddir)/vapi/ \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg x11 \
  $(MAINTAINER_VALAFLAGS)

libunity_static_la_LIBADD = \
  $(BASE_LIBS)

libunity_static_la_SOURCES = \
  unity.vala \
  window.vala

# unity binary
unity_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I quicklauncher/

unity_VALAFLAGS = \
  --vapidir=./quicklauncher \
  --vapidir=./ \
  --vapidir=$(top_builddir)/libunity/ \
  --vapidir=$(top_builddir)/vapi/ \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg clutk-0.3 \
  --pkg launcher-0.3 \
  --pkg unity \
  --pkg unity-static \
  --pkg unity-quicklauncher \
  $(MAINTAINER_VALAFLAGS)

unity_LDADD = \
  libunity-static.la \
  quicklauncher/libunity-quicklauncher.la \
  $(BASE_LIBS)

unity_SOURCES = \
  main.vala

CLEANFILES = *.stamp
