NULL =
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
  unity-files-daemon

unity_files_daemon_CPPFLAGS = \
  -DDATADIR=\"$(DATADIR)\" \
  -DPKGDATADIR=\"$(PKGDATADIR)\" \
  -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
  -DG_LOG_DOMAIN=\"unity-files-daemon\" \
  $(PLACE_DAEMON_CFLAGS) \
  $(MAINTAINER_CFLAGS) \
  -I$(top_srcdir)/common \
  -I$(top_srcdir)/files

unity_files_daemon_VALAFLAGS = \
  -C \
  --vapidir=./ \
  --vapidir=$(top_srcdir)/vapi/ \
  --pkg config \
  --pkg dbusmodel \
  --pkg zeitgeist-1.0 \
  $(MAINTAINER_VALAFLAGS)

unity_files_daemon_LDADD = \
  $(PLACE_DAEMON_LIBS) \
  $(NULL)

# Vala Sources
common_sources = \
  $(NULL)

unity_files_daemon_VALASOURCES = \
  $(common_sources) \
  files/main.vala \
  $(NULL)

unity_files_daemon_SOURCES = \
  $(unity_files_daemon_VALASOURCES:.vala=.c) \
  $(NULL)

BUILT_SOURCES += unity_files_daemon.vala.stamp
EXTRA_DIST += \
  unity_files_daemon.vala.stamp \
  $(unity_files_daemon_VALASOURCES) \
  $(NULL)

unity_files_daemon.vala.stamp: $(unity_files_daemon_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(unity_files_daemon_VALAFLAGS) $^
	touch unity_files_daemon.vala.stamp

CLEANFILES += \
  *.stamp \
  *.vapi \
  $(unity_files_daemon_VALASOURCES:.vala=.c) \
  $(NULL)
