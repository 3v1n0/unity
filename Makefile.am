SUBDIRS = data libunity unity-places src mutter-plugin tests tools vapi po

# Test reports
.PHONY: check-report
check-report:
	$(MAKE) -C tests/ $(@)

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall --enable-mutter

EXTRA_DIST = \
  autogen.sh \
  COPYING
