m4_define([unity_major], [0])
m4_define([unity_minor], [2])
m4_define([unity_micro], [15])

m4_define([unity_api],
          [unity_major.unity_minor])
m4_define([unity_version],
          [unity_major.unity_minor.unity_micro])

m4_define([unity_interface_age], [0])
m4_define([unity_binary_age],
          [m4_eval(100 * unity_minor + unity_micro)])

AC_INIT([unity],[unity_version],[https://launchpad.net/unity])
AC_COPYRIGHT([Copyright 2009 Canonical])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ(2.59)

AC_CONFIG_HEADERS([config.h])
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
AC_HEADER_STDC

LT_INIT
AC_CONFIG_MACRO_DIR([m4])

dnl Gettext
GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/po/LINGUAS'])
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])
IT_PROG_INTLTOOL([0.35.0])

dnl Check for perceptualdiff
AC_CHECK_PROG(HAVE_IMAGEDIFF, perceptualdiff, true, false)
if test "x${HAVE_IMAGEDIFF}" = "xfalse"; then
  AC_MSG_WARN([*** `perceptualdiff` not found, required for UI tests])
fi

dnl Setup a few consts
if test "x${prefix}" = "xNONE"; then
  PREFIX="${ac_default_prefix}"
else
  PREFIX="${prefix}"
fi

dnl GConf schema installation
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

AC_DEFINE_UNQUOTED(LOCALE_DIR, "${PREFIX}/${DATADIRNAME}/locale",[Locale directory])
AC_DEFINE_UNQUOTED(DATADIR, "${PREFIX}/${DATADIRNAME}",[Data directory])
AC_DEFINE_UNQUOTED(PREFIXDIR, "${PREFIX}",[Prefix directory])

dnl Use strict compiler flags only on development releases
m4_define([maintainer_flags_default],
          [m4_if(m4_eval(unity_micro % 2), [1], [yes], [no])])
AC_ARG_ENABLE([maintainer-flags],
              [AS_HELP_STRING([--enable-maintainer-flags=@<:@no/yes@:>:],[Use string compiler flags @<:@default=no@:>@])],
              [],
              [enable_maintainer_flags=maintainer_flags_default])
MAINTAINER_CFLAGS=""
MAINTAINER_VALAFLAGS=""
AS_IF([test "x$enable_maintainer_flags" = "xyes" && test "x$GCC" = "xyes"],
      [
        MAINTAINER_CFLAGS="-g -Wcast-align -Wno-uninitialized -Wempty-body -Wformat-security -Winit-self"
        MAINTAINER_VALAFLAGS="-g --save-temps"
      ]
)
AC_SUBST(MAINTAINER_CFLAGS)
AC_SUBST(MAINTAINER_VALAFLAGS)

dnl Check for module and library dependancies
GLIB_REQUIRED=2.22
GTK_REQUIRED=2.18
PKG_CHECK_MODULES(BASE,
                  clutter-1.0 >= 1.0
                  clutter-gtk-0.10 >= 0.10
                  clutk-0.3 >= 0.3.26
                  dbus-glib-1
                  dee-1.0 >= 0.2.0
                  glib-2.0 >= $GLIB_REQUIRED
                  gobject-2.0 >= $GLIB_REQUIRED
                  gtk+-2.0 >= $GTK_REQUIRED
                  gdk-x11-2.0 >= $GTK_REQUIRED
                  gio-2.0 >= $GTK_REQUIRED
                  gee-1.0
                  gconf-2.0
                  gnome-desktop-2.0
                  indicator
                  libbamf >= 0.2
                  libgnomeui-2.0
                  unique-1.0
                  unity-misc
                  x11)

AC_SUBST(BASE_CFLAGS)
AC_SUBST(BASE_LIBS)

dnl testing for logging boot metrics and instrumenting
with_testing="no"
AC_ARG_ENABLE(testing,
              AS_HELP_STRING([--enable-testing],
                             [Enables instrumenting and boot metric logging]),
              with_testing=$enableval,
              with_testing=no)
AM_CONDITIONAL([HAVE_TESTING], [test "x$with_testing" = "xyes"])

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
              [AS_HELP_STRING([--enable-mutter=@<:@no/yes@:>@],[Enable compiling mutter plugin.])],
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

dnl Indicator support
INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`

AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

dnl Create the Makefiles
AC_CONFIG_FILES([
  Makefile
  unity.pc
  data/Makefile
  targets/Makefile
  targets/mutter/Makefile
  targets/unity/Makefile
  tests/Makefile
  tests/ui/Makefile
  tests/ui/data/Makefile
  tests/unit/Makefile
  tests/unit/data/Makefile
  tools/Makefile
  unity/Makefile
  unity-private/Makefile
  vapi/Makefile
  po/Makefile.in
])
AC_OUTPUT

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
