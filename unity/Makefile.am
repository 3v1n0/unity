BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

lib_LTLIBRARIES = \
  libunity.la

##
# Headers and vapi
##
unityincludedir = $(includedir)/unity/unity
unityinclude_HEADERS = \
  unity.h \
  unity-place.h \
  unity-place-marshal.h

unityvapidir = $(datadir)/vala/vapi
unityvapi_DATA = unity.vapi

##
# DBus Specs
##

DBUS_SPECS = \
 unity-place.xml

%-client.h: %.xml
	$(AM_V_GEN)dbus-binding-tool \
		--prefix=_$(subst -,_,$(basename $(notdir $<)))_client \
		--mode=glib-client \
		--output=$@ \
		$<

%-server.h: %.xml
	$(AM_V_GEN)dbus-binding-tool \
		--prefix=_$(subst -,_,$(basename $(notdir $<)))_server \
		--mode=glib-server \
		--output=$@ \
		$<

BUILT_SOURCES += \
	$(DBUS_SPECS:.xml=-client.h) \
	$(DBUS_SPECS:.xml=-server.h)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST += $(DBUS_SPECS)

##
# Signal Marshallers
##
unity-place-marshal.h: $(srcdir)/unity-place-marshal.list
	$(AM_V_GEN)glib-genmarshal --header \
		--prefix=_unity_place_marshal $(srcdir)/unity-place-marshal.list \
	 	> unity-place-marshal.h

unity-place-marshal.c: $(srcdir)/unity-place-marshal.list
	$(AM_V_GEN)glib-genmarshal --body \
		--prefix=_unity_place_marshal $(srcdir)/unity-place-marshal.list \
		> unity-place-marshal.c

BUILT_SOURCES += \
	unity-place-marshal.c \
	unity-place-marshal.h

EXTRA_DIST += unity-place-marshal.list

##
# libunity
##
if HAVE_LOCALINSTALL
PKGDATADIR=$(top_srcdir)/data
else
PKGDATADIR=$(datadir)/unity
endif

libunity_la_CPPFLAGS = \
  -DG_LOG_DOMAIN=\"libunity\" \
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
  --pkg gconf-2.0 \
  --pkg gtk+-2.0 \
  --pkg gee-1.0 \
  --pkg Bamf-0.2 \
  --pkg unique-1.0 \
  --pkg unity-const \
  $(MAINTAINER_VALAFLAGS)

libunity_la_SOURCES = \
  unity-place.c \
  unity-place-client.h \
  unity-place-marshal.c \
  unity-place-server.h \
  $(libunity_la_VALASOURCES:.vala=.c)

libunity_la_VALASOURCES = \
  chrome-handler.vala \
  drag-controller.vala \
  drag-view.vala \
  entry.vala \
  icon-postprocessor.vala \
  prism-handler.vala \
  quicklist-rendering.vala \
  shell.vala \
  theme.vala \
  unity-favorites.vala \
  webapp-fetcher.vala


libunity_la_APIFILES = \
  unity.h \
  unity.vapi

BUILT_SOURCES += libunity_la_vala.stamp
EXTRA_DIST += \
  $(BUILT_SOURCES) \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES)

libunity_la_vala.stamp $(libunity_la_APIFILES): $(libunity_la_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(libunity_la_VALAFLAGS) $^
	touch libunity_la_vala.stamp

CLEANFILES += \
  *.stamp \
  $(libunity_la_APIFILES) \
  $(libunity_la_VALASOURCES:.vala=.c) \
  *.vapi
