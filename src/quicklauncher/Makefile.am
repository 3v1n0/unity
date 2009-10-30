DATADIR = $(datadir)
PKGDATADIR = $(datadir)/unity

noinst_LTLIBRARIES = \
  libunity-quicklauncher.la

# unity-quicklauncher static lib
libunity_quicklauncher_la_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_builddir)/libunity/

libunity_quicklauncher_la_VALAFLAGS = \
  -H unity-quicklauncher.h -d . \
  --library unity-quicklauncher \
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

libunity_quicklauncher_la_LIBADD = \
  $(BASE_LIBS) \
  $(top_builddir)/libunity/libunity.la

libunity_quicklauncher_la_SOURCES = \
  application_view.vala \
  application_store.vala \
  quicklauncher.vala
  

CLEANFILES = *.stamp
