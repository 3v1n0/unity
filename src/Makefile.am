DATADIR = $(datadir)

SUBDIRS = \
  quicklauncher \
  places

noinst_LTLIBRARIES = \
  libunity-static.la 

bin_PROGRAMS = \
  unity

# unity static lib 
libunity_static_la_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_builddir)/libunity/ \
  -I quicklauncher/ \
  -I places/

libunity_static_la_VALAFLAGS = \
  -H unity-static.h -d . \
  --library unity-static \
  --vapidir=$(top_builddir)/vapi/ \
  --vapidir=$(top_builddir)/libunity/ \
  --vapidir=./quicklauncher/ \
  --vapidir=./places/ \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg gconf-2.0 \
  --pkg launcher-0.3 \
  --pkg unique-1.0 \
  --pkg libwnck-1.0 \
  --pkg x11 \
  --pkg unity \
  --pkg unity-quicklauncher \
  --pkg unity-places \
  $(MAINTAINER_VALAFLAGS)

libunity_static_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la \
  quicklauncher/libunity-quicklauncher.la \
  places/libunity-places.la

libunity_static_la_SOURCES = \
  unity-utils.c \
  application.vala \
  background.vala \
  unity.vala \
  utils.vala \
  window.vala

# unity binary
unity_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_builddir)/libunity/

unity_VALAFLAGS = \
  --vapidir=./ \
  --vapidir=$(top_builddir)/libunity/ \
  --vapidir=$(top_builddir)/vapi/ \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-static \
  $(MAINTAINER_VALAFLAGS)

unity_LDADD = \
  libunity-static.la \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

unity_SOURCES = \
  main.vala

CLEANFILES = *.stamp
