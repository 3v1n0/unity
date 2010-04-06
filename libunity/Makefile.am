BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

noinst_LTLIBRARIES = \
  libunity.la

if HAVE_LOCALINSTALL
PKGDATADIR=$(top_srcdir)/data
else
PKGDATADIR=$(datadir)/unity
endif

# unity-quicklauncher static lib
libunity_la_CPPFLAGS = \
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
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg unique-1.0 \
  --pkg unity-const \
  $(MAINTAINER_VALAFLAGS)

libunity_la_SOURCES = \
  $(libunity_la_VALASOURCES:.vala=.c)

libunity_la_VALASOURCES = \
  application.vala \
  chrome-handler.vala \
  drag-controller.vala \
  drag-view.vala \
  entry.vala \
  icon-postprocessor.vala \
  object-registry.vala \
  perf-logger.vala \
  prism-handler.vala \
  shell.vala \
  theme.vala \
  webapp-fetcher.vala

libunity_la_APIFILES = \
  unity.h \
  unity.vapi

BUILT_SOURCES += libunity_la_vala.stamp
EXTRA_DIST += \
  $(BUILT_SOURCES) \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES)

$(BUILT_SOURCES) $(libunity_la_APIFILES): $(libunity_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_la_VALAFLAGS) $^
	touch libunity_la_vala.stamp

CLEANFILES += \
  *.stamp \
  *.h \
  *.c \
  *.vapi
