SUBDIRS = data libunity src tests

# Test reports
.PHONY: check-report
check-report:
	$(MAKE) -C tests/ $(@)

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall
