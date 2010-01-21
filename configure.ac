m4_define([unity_major], [0])
m4_define([unity_minor], [1])
m4_define([unity_micro], [8])

m4_define([unity_api],
          [unity_major.unity_minor])
m4_define([unity_version],
          [unity_major.unity_minor.unity_micro])

m4_define([unity_interface_age], [0])
m4_define([unity_binary_age],
          [m4_eval(100 * unity_minor + unity_micro)])

AC_INIT([unity],
        [unity_version],
        [https://launchpad.net/unity])
AC_COPYRIGHT([Copyright 2009 Canonical])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ(2.59)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(unity, unity_version)

dnl Get version number into Autoconf
DL_MAJOR_VERSION=unity_major
DL_MINOR_VERSION=unity_minor
DL_MICRO_VERSION=unity_micro
DL_VERSION=unity_version
AC_SUBST(DL_MAJOR_VERSION)
AC_SUBST(DL_MINOR_VERSION)
AC_SUBST(DL_MICRO_VERSION)
AC_SUBST(DL_VERSION)

dnl Init the other things we depend on
AM_MAINTAINER_MODE
AM_PROG_VALAC([0.7.8])
AS_IF([test -z "$VALAC"], [AC_MSG_ERROR(["No valac compiler found."])])
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS

AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

dnl Use strict compiler flags only on development releases
m4_define([maintainer_flags_default],
          [m4_if(m4_eval(unity_minor % 2), [1], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AC_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>:],
                              [Use string compiler flags @<:@default=no@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])
MAINTAINER_CFLAGS=""
MAINTAINER_VALAFLAGS=""
AS_IF([test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
      [
        MAINTAINER_CFLAGS="-g -Wall -Werror -Wshadow -Wcast-align -Wno-uninitialized -Wempty-body -Wformat-security -Winit-self"
        MAINTAINER_VALAFLAGS="-g --save-temps"
      ]
)
AC_SUBST(MAINTAINER_CFLAGS)
AC_SUBST(MAINTAINER_VALAFLAGS)

dnl Check for module and library dependancies
GLIB_REQUIRED=2.22
GTK_REQUIRED=2.18
PKG_CHECK_MODULES(BASE,
                  glib-2.0 >= $GLIB_REQUIRED
                  gobject-2.0 >= $GLIB_REQUIRED
                  gtk+-2.0 >= $GTK_REQUIRED
                  gdk-x11-2.0 >= $GTK_REQUIRED
                  gio-2.0 >= $GTK_REQUIRED
                  gee-1.0
                  gconf-2.0
                  clutter-1.0 >= 1.0
                  clutter-gtk-0.10 >= 0.10
                  clutk-0.3 >= 0.3
                  launcher-0.3 >= 0.3
                  libwnck-1.0 >= 2.28.0
                  unique-1.0
                  x11
                  )

dnl testing for logging boot metrics and instrumenting
with_testing="no"
AC_ARG_ENABLE(testing,
              AS_HELP_STRING([--enable-testing],
                             [Enables instrumenting and boot metric logging]),
              with_testing=$enableval,
              with_testing=no)
AM_CONDITIONAL([HAVE_TESTING], [test "x$with_testing" = "xyes"])

AC_SUBST(BASE_CFLAGS)
AC_SUBST(BASE_LIBS)

dnl local install for distcheck and stand-alone running
with_localinstall="no"
AC_ARG_ENABLE(localinstall,
              AS_HELP_STRING([--enable-localinstall],
                             [Install all of the files locally instead of in system directories (for distcheck)]),
              with_localinstall=$enableval,
              with_localinstall=no)

AM_CONDITIONAL([HAVE_LOCALINSTALL], [test "x$with_localinstall" = "xyes"])

dnl Mutter support
AC_ARG_ENABLE([mutter],
              [AC_HELP_STRING([--enable-mutter=@<:@no/yes@:>@],
                              [Enable compiling mutter plugin.])],
              [enable_mutter=$enableval],
              [enable_mutter=yes])
AS_IF([test "x$enable_mutter" = "xyes"],
      [
        AC_DEFINE([ENABLE_MUTTER], 1, [Enable Mutter plugin])
        PKG_CHECK_MODULES(MUTTER, mutter-plugins)
        AC_SUBST(MUTTER_CFLAGS)
        AC_SUBST(MUTTER_LIBS)
        MUTTER_PLUGIN_DIR=`$PKG_CONFIG --variable=plugindir mutter-plugins`
        AC_SUBST(MUTTER_PLUGIN_DIR)
      ]
)
AM_CONDITIONAL(ENABLE_MUTTER, [test "x$enable_mutter" = "xyes"])

dnl Create the Makefiles
AC_OUTPUT([
  Makefile
  data/Makefile
  libunity/Makefile
  src/Makefile
  mutter-plugin/Makefile
  tests/Makefile
  tools/Makefile
  vapi/Makefile
])

dnl Output the results
AC_MSG_NOTICE([

  Unity $VERSION
  ----------------------

  Prefix         : ${prefix}

  Localinstall   : ${with_localinstall}

  Mutter plugin  : ${enable_mutter}

  Extra CFlags   : ${CPPFLAGS} $MAINTAINER_CFLAGS
  Extra ValaFlags: ${CPPFLAGS} $MAINTAINER_VALAFLAGS
])
