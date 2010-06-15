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
bin_PROGRAMS = \
  unity

unity_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
  -I$(top_srcdir)/unity/ \
  -I$(top_srcdir)/unity-private/

unity_VALAFLAGS = \
  -C \
  --vapidir=./ \
  --vapidir=$(top_srcdir)/unity/ \
  --vapidir=$(top_srcdir)/unity-private/ \
  --vapidir=$(top_srcdir)/vapi/ \
  --thread \
  --pkg Bamf-0.2 \
  --pkg clutk-0.3 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg cogl-1.0 \
  --pkg config \
  --pkg dee-1.0 \
  --pkg dbus-glib-1 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg indicator \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unity-private \
  $(MAINTAINER_VALAFLAGS)

unity_LDADD = \
  $(BASE_LIBS) \
  $(top_builddir)/unity/libunity.la \
  $(top_builddir)/unity-private/libunity-private.la

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
  $(unity_VALASOURCES:.vala=.c)
