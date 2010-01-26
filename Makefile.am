SUBDIRS = data libunity libunity-places src mutter-plugin tests tools vapi

# Test reports
.PHONY: check-report
check-report:
	$(MAKE) -C tests/ $(@)

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall --enable-mutter
