AM_CFLAGS = \
     $(LOCPLA_CFLAGS) \
     $(GPSUTILS_CFLAGS) \
     -I./

ACLOCAL_AMFLAGS = -I m4

libgnsspps_la_SOURCES = \
    gnsspps.c

if USE_GLIB
libgnsspps_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgnsspps_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libgnsspps_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgnsspps_la_CFLAGS = $(AM_CFLAGS)
libgnsspps_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libgnsspps_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libgnsspps_la_LIBADD = -lstdc++ $(GPSUTILS_LIBS)

library_include_HEADERS = \
    gnsspps.h

#Create and Install libraries
lib_LTLIBRARIES = libgnsspps.la

library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnsspps.pc
EXTRA_DIST = $(pkgconfig_DATA)




