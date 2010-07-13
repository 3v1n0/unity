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
lib_LTLIBRARIES = \
  libunity-private.la

libunity_private_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DINDICATORDIR=\""$(INDICATORDIR)"\" \
  -DINDICATORICONSDIR=\""$(INDICATORICONSDIR)"\" \
  -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
  -DG_LOG_DOMAIN=\"libunity-private\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_srcdir)/unity/ \
  -I$(top_srcdir)/src/ \
  -I$(top_srcdir)/places/ \
  -I$(top_srcdir)/launcher/ \
  -I$(top_srcdir)/testing/

libunity_private_la_VALAFLAGS = \
  -C \
  -H unity-private.h -d . \
  --thread \
  --library unity-private \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/unity/ \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg config \
  --pkg cogl-1.0 \
  --pkg dbus-glib-1 \
  --pkg Dbusmenu-Glib-0.2\
  --pkg dee-1.0 \
  --pkg gconf-2.0 \
  --pkg gdk-2.0 \
  --pkg gdk-x11-2.0 \
  --pkg gee-1.0 \
  --pkg gio-unix-2.0 \
  --pkg gtk+-2.0 \
  --pkg indicator \
  --pkg Bamf-0.2 \
  --pkg libwnck-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unity-misc \
  --pkg unique-1.0 \
  --pkg x11 \
  $(MAINTAINER_VALAFLAGS)

libunity_private_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/unity/libunity.la

# Vala Sources
private_sources = \
  application.vala \
  unity.vala \
  utils.vala

panel_sources = \
  panel/panel-background.vala \
  panel/panel-home-button.vala \
  panel/panel-indicator-background.vala \
  panel/panel-indicator-bar.vala \
  panel/panel-indicator-model.vala \
  panel/panel-indicator-object-entry-view.vala \
  panel/panel-indicator-object-view.vala \
  panel/panel-menu-manager.vala \
  panel/panel-menubar.vala \
  panel/panel-tray.vala \
  panel/panel-view.vala

places_sources = \
  places/places-controller.vala \
  places/places-default-renderer-group.vala \
  places/places-default-renderer.vala \
  places/places-place-bar.vala \
  places/places-place-entry-view.vala \
  places/places-place-entry.vala \
  places/places-place-model.vala \
  places/places-place-view.vala \
  places/places-place.vala \
  places/places-place-search-bar.vala \
  places/places-place-search-entry.vala \
  places/places-place-search-sections-bar.vala \
  places/places-view.vala

launcher_sources = \
  launcher/application-controller.vala \
  launcher/launcher.vala \
  launcher/launcher-child.vala \
  launcher/quicklist-check-menu-item.vala \
  launcher/quicklist-controller.vala \
  launcher/quicklist-image-menu-item.vala \
  launcher/quicklist-menu-item.vala \
  launcher/quicklist-radio-menu-item.vala \
  launcher/quicklist-seperator-menu-item.vala \
  launcher/quicklist-view.vala \
  launcher/scrollerchild-controller.vala \
  launcher/scrollerchild.vala \
  launcher/scroller-controller.vala \
  launcher/scroller-model.vala \
  launcher/scroller-view.vala

testing_sources = \
  testing/background.vala \
  testing/object-registry.vala \
  testing/perf-logger.vala \
  testing/test-director.vala \
  testing/test-utils.vala \
  testing/test-window.vala

libunity_private_la_VALASOURCES = \
  $(private_sources) \
  $(panel_sources) \
  $(places_sources) \
  $(launcher_sources) \
  $(testing_sources)

libunity_private_la_SOURCES = \
  unity-utils.c \
  unity-utils.h \
  $(libunity_private_la_VALASOURCES:.vala=.c)

libunity_private_la_APIFILES = unity-private.vapi unity-private.h unity-utils.h

BUILT_SOURCES += libunity_private_la.vala.stamp
EXTRA_DIST += \
  libunity_private_la.vala.stamp \
  $(libunity_private_la_APIFILES) \
  $(libunity_private_la_VALASOURCES)

libunity_private_la.vala.stamp $(libunity_private_la_APIFILES): $(libunity_private_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_private_la_VALAFLAGS) $^
	touch libunity_private_la.vala.stamp

CLEANFILES += \
  *.stamp \
  *.vapi \
  $(libunity_private_la_VALASOURCES:.vala=.c) \
  unity-private.h
