DATADIR = $(datadir)
PKGDATADIR = $(datadir)/unity

noinst_LTLIBRARIES = \
  libunity-homescreen.la

# unity-homescreen static lib
libunity_homescreen_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_builddir)/libunity/

libunity_homescreen_la_VALAFLAGS = \
  -H unity-homescreen.h -d . \
  --library unity-homescreen \
  --vapidir=$(top_builddir)/vapi/ \
  --vapidir=$(top_builddir)/libunity \
  --pkg clutter-1.0 \
  --pkg clutk-0.3 \
  --pkg launcher-0.3 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg unity-const \
  --pkg unity \
  $(MAINTAINER_VALAFLAGS)

libunity_homescreen_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

libunity_homescreen_la_SOURCES = \
  default_model.vala \
  default_view.vala \
  places_model.vala \
  places_view.vala \
  activity_view.vala \
  activity_store.vala \
  homescreen.vala

CLEANFILES = *.stamp
