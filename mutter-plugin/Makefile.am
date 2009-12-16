mutterlibdir = $(MUTTER_PLUGIN_DIR)
mutterlib_LTLIBRARIES = \
  libunity-mutter.la

# unity-quicklauncher static lib
libunity_mutter_la_CPPFLAGS = \
  -I$(top_builddir)/libunity \
  -I$(top_builddir)/src \
  $(BASE_CFLAGS) \
  $(MUTTER_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_mutter_la_VALAFLAGS = \
  -H unity-mutter.h -d . \
  --library unity-mutter \
  --vapidir=$(top_builddir)/vapi/ \
  --vapidir=$(top_builddir)/src/ \
  --vapidir=$(top_builddir)/libunity \
  --pkg clutter-1.0 \
  --pkg clutk-0.3 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg mutter-2.28 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unique-1.0 \
  --pkg x11 \
  --pkg unity-static \
  --pkg unity \
  $(MAINTAINER_VALAFLAGS)

libunity_mutter_la_LIBADD = \
  $(top_builddir)/src/libunity-static.la \
  $(BASE_LIBS) \
  $(MUTTER_LIBS)

libunity_mutter_la_SOURCES = \
  plugin.vala \
  window-management.vala \
  main.c

CLEANFILES = \
  *.stamp
