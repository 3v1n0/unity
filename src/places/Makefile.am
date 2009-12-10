DATADIR = $(datadir)
PKGDATADIR = $(datadir)/unity

noinst_LTLIBRARIES = \
  libunity-places.la

# unity-places static lib
libunity_places_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_builddir)/libunity/

libunity_places_la_VALAFLAGS = \
  -H unity-places.h -d . \
  --library unity-places \
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

libunity_places_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

libunity_places_la_SOURCES = \
  bar_model.vala \
  bar_view.vala \
  default_model.vala \
  default_view.vala \
  view.vala

CLEANFILES = *.stamp
