SUBDIRS = data

noinst_PROGRAMS = test-unit

TESTDIR = $(top_srcdir)/tests/unit

test_unit_CPPFLAGS = \
  -I$(top_srcdir) \
  -I$(top_srcdir)/unity \
  -I$(top_srcdir)/unity-private \
  -I$(top_srcdir)/src \
  -DTESTDIR=\"$(TESTDIR)\" \
  $(BASE_CFLAGS)

test_unit_LDADD = \
  $(top_builddir)/unity/libunity.la \
  $(top_builddir)/unity-private/libunity-private.la \
  $(BASE_LIBS) \
  $(MAINTAINER_LIBS)

test_unit_VALAFLAGS = \
  --vapidir=$(top_srcdir)/unity \
  --vapidir=$(top_srcdir)/unity-private \
  --vapidir=$(top_srcdir)/src \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/tests \
  --vapidir=$(top_srcdir)/tests/unit \
  --pkg gio-2.0 \
  --pkg Bamf-0.2 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg dbus-glib-1 \
  --pkg dee-1.0 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg indicator \
  --pkg clutk-0.3 \
  --pkg launcher-0.3 \
  --pkg posix \
  --pkg test-const \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unity-private \
  $(MAINTAINER_VALAFLAGS)

test_unit_SOURCES = \
  test-launcher.vala \
  test-panel-indicator-object-entry-view.vala \
  test-panel-indicator-object-view.vala \
  test-places-place-file-model.vala \
  test-places-place.vala \
  test-places.vala \
  test-place.vala \
  test-io.vala \
  test-appinfo-manager.vala \
  test-unity-pixbuf-cache.vala \
  test-unit.vala

# GTester stuff
XML_REPORT = test-unit-results.xml
HTML_REPORT = test-unit-results.html

unit-tester: test-unit
	@gtester -o=$(XML_REPORT) ./test-unit

check-report:
	@gtester -o=$(XML_REPORT) -k ./test-unit \
	&& (gtester-report $(XML_REPORT) \
			| sed 's/GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
			> $(HTML_REPORT)) \
	&& xdg-open $(HTML_REPORT)

check-local: unit-tester

EXTRA_DIST = test-const.vapi
CLEANFILES = *.stamp $(XML_REPORT) $(HTML_REPORT) *.c
