# Please keep alphabetical :-)

dist_default_DATA = \
  applications.png \
  bfb.png \
  dash_background.png \
  devices.png \
  favourites.png \
  files.png \
  gradient.png \
  home.png \
  honeycomb.png \
  honeycomb-mask.png \
  maximize_up.png \
  minimize_up.png \
  quicklauncher_focused_indicator.png \
  quicklauncher_running_indicator.png \
  scroller-fade.png \
  scroller-shadow.png \
  search_icon.png \
  software_centre.png \
  tight_check_4px.png \
  trash.png

defaultdir = $(pkgdatadir)

# Schemas
schemadir = $(sysconfdir)/gconf/schemas
dist_schema_DATA = unity.schemas

if GCONF_SCHEMAS_INSTALL
install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(dist_schema_DATA)
endif

if GCONF_SCHEMAS_INSTALL
uninstall-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule $(dist_schema_DATA)
endif
