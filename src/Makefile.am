BUILT_SOURCES = 
CLEANFILES = 
EXTRA_DIST = 
  
DATADIR = $(datadir)
PKGDATADIR = $(pkgdatadir)

noinst_LTLIBRARIES = \
  libunity-static.la 

bin_PROGRAMS = \
  unity

# unity static lib 
libunity_static_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_srcdir)/libunity/ \
  -I$(top_srcdir)/src
	-I$(top_srcdir)/places/ \
  -I$(top_srcdir)/quicklauncher/

libunity_static_la_VALAFLAGS = \
  -C \
  -H unity-static.h -d . \
  --library unity-static \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/libunity/ \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gconf-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg gio-unix-2.0 \
  --pkg gtk+-2.0 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unique-1.0 \
  --pkg x11 \
  $(MAINTAINER_VALAFLAGS)

libunity_static_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

# Vala Sources
src_sources = \
  application.vala \
  background.vala \
  unity.vala \
  utils.vala \
  window.vala

places_sources = \
  places/bar-model.vala \
  places/bar-view.vala \
  places/default-model.vala \
  places/default-view.vala \
  places/view.vala

quicklauncher_sources = \
  quicklauncher/application-store.vala \
  quicklauncher/launcher-store.vala \
  quicklauncher/launcher-view.vala \
  quicklauncher/prism-handler.vala \
  quicklauncher/quicklauncher-manager.vala \
  quicklauncher/quicklauncher.vala \
  quicklauncher/tooltipmanager.vala \
  quicklauncher/unity-scroller.vala

libunity_static_la_VALASOURCES = \
  $(src_sources) \
  $(places_sources) \
  $(quicklauncher_sources)

libunity_static_la_SOURCES = \
  unity-utils.c \
  $(libunity_static_la_VALASOURCES:.vala=.c)

libunity_static_la_APIFILES = unity-static.vapi unity-static.h

BUILT_SOURCES += libunity_static_la.vala.stamp
EXTRA_DIST += \
  libunity_static_la.vala.stamp \
  $(libunity_static_la_APIFILES) \
  $(libunity_static_la_VALASOURCES)

libunity_static_la.vala.stamp $(libunity_static_la_APIFILES): $(libunity_static_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_static_la_VALAFLAGS) $^
	touch libunity_static_la.vala.stamp

# unity binary
unity_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_srcdir)/libunity/

unity_VALAFLAGS = \
  -C \
  --vapidir=./ \
  --vapidir=$(top_srcdir)/libunity/ \
  --vapidir=$(top_srcdir)/vapi/ \
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

unity_VALASOURCES = \
  main.vala

unity_SOURCES = \
  $(unity_VALASOURCES:.vala=.c)

BUILT_SOURCES += unity.vala.stamp
EXTRA_DIST += unity.vala.stamp $(unity_VALASOURCES)

unity.vala.stamp: $(unity_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(unity_VALAFLAGS) $^
	touch unity.vala.stamp

CLEANFILES += *.stamp *.h *.vapi quicklauncher/*.c places/*.c
