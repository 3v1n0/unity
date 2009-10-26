bin_PROGRAMS = test-unity

test_unity_CPPFLAGS = \
  -I$(top_builddir)/src \
  -I$(top_builddir)/src/quicklauncher \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

test_unity_LDADD = \
  $(top_builddir)/src/libunity-static.la \
  $(top_builddir)/src/quicklauncher/libunity-quicklauncher.la \
  $(BASE_LIBS) \
  $(MAINTAINER_LIBS)

test_unity_VALAFLAGS = \
  --vapidir=$(top_builddir)/src \
  --vapidir=$(top_builddir)/src/quicklauncher \
  --vapidir=$(top_builddir)/vapi/ \
  --vapidir=./ \
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
  --pkg unity-static \
  --pkg unity-quicklauncher \
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

CLEANFILES = *.stamp $(XML_REPORT) $(HTML_REPORT)
