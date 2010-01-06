SUBDIRS = data libunity src mutter-plugin tests

# Test reports
.PHONY: check-report
check-report:
	$(MAKE) -C tests/ $(@)

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall --enable-mutter
