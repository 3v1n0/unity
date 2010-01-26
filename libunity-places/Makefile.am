BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unity-places.pc

placesincludedir = $(includedir)/unity-places/unity-places

places_headers = \
  unity-places.h \
  unity-place.h \
  unity-place-client.h \
  unity-place-proxy.h \
  unity-place-server.h

placesinclude_HEADERS = \
  $(places_headers)

lib_LTLIBRARIES = libunity-places.la

libunity_places_la_SOURCES = \
  unity-place.c \
  unity-place-proxy.c

libunity_places_la_LIBADD = \
  $(BASE_LIBS)

libunity_places_la_LDFLAGS = \
  $(BASE_LT_LDFLAGS)

libunity_places_la_CPPFLAGS = \
  -I$(srcdir) \
  -I$(top_srcdir) \
  -DG_LOG_DOMAIN=\"libunity-places\" \
  -DPREFIX=\""$(prefix)"\" \
  -DLIBDIR=\""$(libdir)"\" \
  -DG_DISABLE_DEPRECATED \
  $(GCC_FLAGS) \
  $(BASE_CFLAGS) \
  $(MAINTAINER_CFLAGS)

##
# DBus Specs
##

DBUS_SPECS = \
 unity-place.xml

%-client.h: %.xml
	dbus-binding-tool \
		--prefix=_$(subst -,_,$(basename $(notdir $<)))_client \
		--mode=glib-client \
		--output=$@ \
		$<

%-server.h: %.xml
	dbus-binding-tool \
		--prefix=_$(subst -,_,$(basename $(notdir $<)))_server \
		--mode=glib-server \
		--output=$@ \
		$<

BUILT_SOURCES += \
	$(DBUS_SPECS:.xml=-client.h) \
	$(DBUS_SPECS:.xml=-server.h)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST += $(DBUS_SPECS)
