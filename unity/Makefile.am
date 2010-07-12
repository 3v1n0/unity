BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

lib_LTLIBRARIES = \
  libunity.la

##
# Headers and vapi
##
unityincludedir = $(includedir)/unity/unity
unityinclude_HEADERS = \
  unity.h

unityvapidir = $(datadir)/vala/vapi
unityvapi_DATA = unity.vapi


##
# libunity
##
if HAVE_LOCALINSTALL
PKGDATADIR=$(top_srcdir)/data
else
PKGDATADIR=$(datadir)/unity
endif

libunity_la_CPPFLAGS = \
  -DG_LOG_DOMAIN=\"libunity\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_la_LIBADD = \
  $(BASE_LIBS)

libunity_la_VALAFLAGS = \
  -C \
  -H unity.h -d . \
  --library unity \
  --vapidir=$(top_srcdir)/vapi/ \
  --thread \
  --pkg config \
  --pkg clutter-1.0 \
  --pkg clutk-0.3 \
  --pkg dbus-glib-1 \
  --pkg Dbusmenu-Glib-0.2 \
  --pkg dee-1.0 \
  --pkg gconf-2.0 \
  --pkg gtk+-2.0 \
  --pkg gio-2.0 \
  --pkg gio-unix-2.0 \
  --pkg gee-1.0 \
  --pkg Bamf-0.2 \
  --pkg unique-1.0 \
  --pkg unity-const \
  $(MAINTAINER_VALAFLAGS)

libunity_la_SOURCES = \
  $(libunity_la_VALASOURCES:.vala=.c)

libunity_la_VALASOURCES = \
  chrome-handler.vala \
  drag-controller.vala \
  drag-view.vala \
  entry.vala \
  icon-postprocessor.vala \
  prism-handler.vala \
  quicklist-rendering.vala \
  shell.vala \
  theme.vala \
  unity-appinfo-manager.vala \
  unity-cairo-canvas.vala \
  unity-favorites.vala \
  unity-io.vala \
  unity-pixbuf-cache.vala \
  unity-place-renderer.vala \
  unity-place.vala \
  webapp-fetcher.vala


libunity_la_APIFILES = \
  unity.h \
  unity.vapi

BUILT_SOURCES += libunity_la_vala.stamp
EXTRA_DIST += \
  $(BUILT_SOURCES) \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES)

libunity_la_vala.stamp $(libunity_la_APIFILES): $(libunity_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_la_VALAFLAGS) $^
	touch libunity_la_vala.stamp

CLEANFILES += \
  *.stamp \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES:.vala=.c) \
  *.vapi
