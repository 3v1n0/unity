SUBDIRS = data

noinst_PROGRAMS = test-ui

TESTDIR = $(top_srcdir)/tests/ui

test_ui_CPPFLAGS = \
  -I$(top_srcdir) \
  -I$(top_srcdir)/unity \
  -I$(top_srcdir)/unity-private \
  -I$(top_srcdir)/src \
  -DTESTDIR=\"$(TESTDIR)\" \
  $(BASE_CFLAGS)

test_ui_LDADD = \
  $(top_builddir)/unity/libunity.la \
  $(top_builddir)/unity-private/libunity-private.la \
  $(BASE_LIBS) \
  $(MAINTAINER_LIBS)

test_ui_VALAFLAGS = \
  --vapidir=$(top_srcdir)/unity \
  --vapidir=$(top_srcdir)/unity-private \
  --vapidir=$(top_srcdir)/src \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/tests/ui \
  --pkg Bamf-0.2 \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg dbus-glib-1 \
  --pkg Dbusmenu-Glib-0.2 \
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
  --pkg test-const \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-const \
  --pkg unity-private \
  $(MAINTAINER_VALAFLAGS)

test_ui_SOURCES = \
  test-automate.vala \
  test-home-button.vala \
  test-indicators.vala \
  test-quicklist.vala \
  test-ui.vala

# GTester stuff
XML_REPORT = test-ui-results.xml
HTML_REPORT = test-ui-results.html

unit-tester: test-ui
	@gtester -o=$(XML_REPORT) ./test-ui

check-report:
	@gtester -o=$(XML_REPORT) -k ./test-ui \
	&& (gtester-report $(XML_REPORT) \
			| sed 's/GTester UI Test Report</>GTester UI Test Report (normal)</' \
			> $(HTML_REPORT)) \
	&& xdg-open $(HTML_REPORT)

check-local: unit-tester

EXTRA_DIST = test-const.vapi
CLEANFILES = *.stamp $(XML_REPORT) $(HTML_REPORT) *.c
