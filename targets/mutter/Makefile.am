BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

if ENABLE_MUTTER

if HAVE_TESTING
BASE_CFLAGS+=-DTESTING
endif

if HAVE_LOCALINSTALL
mutterlibdir = /tmp/mutter-plugin/
else
mutterlibdir = $(MUTTER_PLUGIN_DIR)
endif

mutterlib_LTLIBRARIES = \
  libunity-mutter.la

# unity-quicklauncher static lib
libunity_mutter_la_CPPFLAGS = \
  -I$(top_srcdir)/unity \
  -I$(top_srcdir)/unity-private \
  -I$(top_srcdir)/src \
  $(BASE_CFLAGS) \
  $(MUTTER_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_mutter_la_VALAFLAGS = \
  -C \
  -H unity-mutter.h -d . \
  --library unity-mutter \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/src/ \
  --vapidir=$(top_srcdir)/unity \
  --vapidir=$(top_srcdir)/unity-private \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg clutk-0.3 \
  --pkg dbus-glib-1 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg indicator \
  --pkg mutter-2.28 \
  --pkg LibBamf-0.2 \
  --pkg libwnck-1.0 \
  --pkg unique-1.0 \
  --pkg x11 \
  --pkg unity-const \
  --pkg unity \
  --pkg unity-private \
  $(MAINTAINER_VALAFLAGS)

libunity_mutter_la_LIBADD = \
  $(top_builddir)/unity/libunity.la \
  $(top_builddir)/unity-private/libunity-private.la \
  $(BASE_LIBS) \
  $(MUTTER_LIBS)

libunity_mutter_la_SOURCES = \
  main.c \
  mutter-plugins.h \
  unity-mutter.h \
  $(libunity_mutter_la_VALASOURCES:.vala=.c)

libunity_mutter_la_VALASOURCES = \
  expose-manager.vala \
  plugin.vala \
  maximus.vala \
  window-management.vala

libunity_mutter_la_APIFILES = \
  unity-mutter.h \
  unity-mutter.vapi

BUILT_SOURCES += libunity_mutter_la.vala.stamp
EXTRA_DIST += \
  $(BUILT_SOURCES) \
  $(libunity_mutter_la_APIFILES) \
  $(libunity_mutter_la_VALASOURCES)

$(BUILT_SOURCES) $(libunity_mutter_la_APIFILES): $(libunity_mutter_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_mutter_la_VALAFLAGS) $^
	touch libunity_mutter_la.vala.stamp

CLEANFILES += \
  *.stamp \
  unity-mutter.vapi \
  $(libunity_mutter_la_VALASOURCES:.vala=.c) \
  $(libunity_mutter_la_APIFILES)

else
endif
