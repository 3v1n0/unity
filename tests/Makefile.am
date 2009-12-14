bin_PROGRAMS = test-unity

TESTDIR = $(top_srcdir)/tests

test_unity_CPPFLAGS = \
  -I$(top_srcdir)/libunity \
  -I$(top_srcdir)/src \
  -DTESTDIR=\"$(TESTDIR)\" \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

test_unity_LDADD = \
  $(top_builddir)/src/libunity-static.la \
  $(BASE_LIBS) \
  $(MAINTAINER_LIBS)

test_unity_VALAFLAGS = \
  --vapidir=$(top_srcdir)/libunity \
  --vapidir=$(top_srcdir)/src \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=$(top_srcdir)/tests \
  --pkg clutter-1.0 \
  --pkg clutter-gtk-0.10 \
  --pkg gtk+-2.0 \
  --pkg gdk-2.0 \
  --pkg gee-1.0 \
  --pkg x11 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg clutk-0.3 \
  --pkg launcher-0.3 \
  --pkg libwnck-1.0 \
  --pkg test-const \
  --pkg unique-1.0 \
  --pkg unity \
  --pkg unity-static \
  $(MAINTAINER_VALAFLAGS)

test_unity_SOURCES = \
  test-unity.vala

# GTester stuff
XML_REPORT = unity-check-results.xml
HTML_REPORT = unity-check-results.html

unity-tester: test-unity
	@gtester -o=$(XML_REPORT) ./test-unity

check-report:
	@gtester -o=$(XML_REPORT) -k ./test-unity \
	&& (gtester-report $(XML_REPORT) \
			| sed 's/GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
			> $(HTML_REPORT)) \
	&& gnome-open $(HTML_REPORT)

check-local: unity-tester

EXTRA_DIST = test-const.vapi firefox.desktop
CLEANFILES = *.stamp $(XML_REPORT) $(HTML_REPORT)
