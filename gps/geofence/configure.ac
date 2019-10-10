AC_PREREQ(2.61)
AC_INIT([location-geofence], 1.0.0)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Check for programs
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

# Check for libraries
PKG_CHECK_MODULES([GPSUTILS], [gps-utils])
AC_SUBST([GPSUTILS_CFLAGS])
AC_SUBST([GPSUTILS_LIBS])

PKG_CHECK_MODULES([LOCCORE], [loc-core])
AC_SUBST([LOCCORE_CFLAGS])
AC_SUBST([LOCCORE_LIBS])

AS_CASE([$host],
        [arm*], [ARM=yes],
        [ARM=no]
)

AC_ARG_WITH([locpla_includes],
      AC_HELP_STRING([--with-locpla-includes=@<:@dir@:>@],
         [specify the path to locpla-includes in loc-pla_git.bb]),
      [locpla_incdir=$withval],
      with_locpla_includes=no)

if test "x$with_locpla_includes" != "xno"; then
   AC_SUBST(LOCPLA_CFLAGS, "-I${locpla_incdir}")
fi

AC_ARG_WITH([glib],
      AC_HELP_STRING([--with-glib],
         [enable glib, building HLOS systems which use glib]))

if (test "x${with_glib}" = "xyes"); then
        AC_DEFINE(ENABLE_USEGLIB, 1, [Define if HLOS systems uses glib])
        PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GThread >= 2.16 is required))
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16, dummy=yes,
                                AC_MSG_ERROR(GLib >= 2.16 is required))
        GLIB_CFLAGS="$GLIB_CFLAGS $GTHREAD_CFLAGS"
        GLIB_LIBS="$GLIB_LIBS $GTHREAD_LIBS"

        AC_SUBST(GLIB_CFLAGS)
        AC_SUBST(GLIB_LIBS)
fi

AM_CONDITIONAL(USE_GLIB, test "x${with_glib}" = "xyes")

AC_SUBST([CFLAGS])
AC_SUBST([CPPFLAGS])
AC_SUBST([LIBS])

AC_CONFIG_FILES([ \
        Makefile \
        location-geofence.pc
        ])

AC_OUTPUT
