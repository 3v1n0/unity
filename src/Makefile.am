BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

DATADIR = $(datadir)
if HAVE_TESTING
BASE_CFLAGS+=-DTESTING
endif

if HAVE_LOCALINSTALL
PKGDATADIR=$(top_srcdir)/data
else
PKGDATADIR=$(datadir)/unity
endif
noinst_LTLIBRARIES = \
  libunity-static.la

bin_PROGRAMS = \
  unity

# unity static lib
libunity_static_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DINDICATORDIR=\""$(INDICATORDIR)"\" \
  -DINDICATORICONSDIR=\""$(INDICATORICONSDIR)"\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_srcdir)/libunity/ \
  -I$(top_srcdir)/src/ \
  -I$(top_srcdir)/places/ \
  -I$(top_srcdir)/quicklauncher/

libunity_static_la_VALAFLAGS = \
  -C \
  -H unity-static.h -d . \
  --thread \
  --library unity-static \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/libunity/ \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg dbus-glib-1 \
  --pkg cogl-1.0 \
  --pkg dbus-glib-1 \
  --pkg gconf-2.0 \
  --pkg gdk-2.0 \
  --pkg gdk-x11-2.0 \
  --pkg gee-1.0 \
  --pkg gio-unix-2.0 \
  --pkg gtk+-2.0 \
  --pkg indicator \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unity-misc \
  --pkg unique-1.0 \
  --pkg x11 \
  $(MAINTAINER_VALAFLAGS)

libunity_static_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

# Vala Sources
src_sources = \
  background.vala \
  unity.vala \
  utils.vala \
  window.vala

panel_sources = \
  panel/panel-home.vala \
  panel/panel-indicators.vala \
  panel/panel-tray.vala \
  panel/panel-view.vala

places_sources = \
  places/application-place.vala \
  places/bar-model.vala \
  places/bar-view.vala \
  places/cairo-drawing.vala \
  places/default-model.vala \
  places/default-view.vala \
  places/file-place.vala \
  places/places-controller.vala \
  places/places-model.vala \
  places/places-place.vala \
  places/places-place-proxy.vala \
  places/places-view.vala

quicklauncher_sources = \
  quicklauncher/application-model.vala \
  quicklauncher/launcher-model.vala \
  quicklauncher/launcher-view.vala \
  quicklauncher/prism-handler.vala \
  quicklauncher/quicklauncher-manager.vala \
  quicklauncher/quicklauncher.vala \
  quicklauncher/quicklist-controller.vala \
  quicklauncher/quicklist-view.vala \
  quicklauncher/tooltipmanager.vala \
  quicklauncher/unity-scroller.vala

libunity_static_la_VALASOURCES = \
  $(src_sources) \
  $(panel_sources) \
  $(places_sources) \
  $(quicklauncher_sources)

libunity_static_la_SOURCES = \
  unity-utils.c \
  unity-utils.h \
  $(libunity_static_la_VALASOURCES:.vala=.c)

libunity_static_la_APIFILES = unity-static.vapi unity-static.h unity-utils.h

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
  --thread \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg cogl-1.0 \
  --pkg dbus-glib-1 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg indicator \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-const \
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

CLEANFILES += \
  *.stamp \
  *.vapi \
  quicklauncher/*.c \
  places/*.c \
  $(unity_VALASOURCES:.vala=.c) \
  $(libunity_static_la_VALASOURCES:.vala=.c) \
  unity-static.h

clean-local:
	rm -f `echo $(quicklauncher_sources:.vala=.c) | sed "s/quicklauncher\///g"`
	rm -f `echo $(places_sources:.vala=.c) | sed "s/places\///g"`
