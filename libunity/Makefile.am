BUILT_SOURCES = 
CLEANFILES = 
EXTRA_DIST = 

noinst_LTLIBRARIES = \
  libunity.la

# unity-quicklauncher static lib
libunity_la_CPPFLAGS = \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

libunity_la_LIBADD = \
  $(BASE_LIBS)

libunity_la_VALAFLAGS = \
	-C \
  -H unity.h -d . \
  --library unity \
  --vapidir=$(top_builddir)/vapi/ \
  --pkg clutter-1.0 \
  --pkg clutk-0.3 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  $(MAINTAINER_VALAFLAGS)

libunity_la_SOURCES = \
  $(libunity_la_VALASOURCES:.vala=.c)

libunity_la_VALASOURCES = \
  shell.vala

libunity_la_APIFILES = \
  unity.vapi

BUILT_SOURCES += libunity_la_vala.stamp
EXTRA_DIST += \
  $(BUILT_SOURCES) \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES)

$(BUILT_SOURCES) $(libunity_la_APIFILES): $(libunity_la_VALASOURCES)
	$(VALAC) $(libunity_la_VALAFLAGS) $^
	touch libunity_la_vala.stamp														

CLEANFILES += \
  *.stamp \
  *.h \
  *.c \
  *.vapi
