AM_CFLAGS = \
     $(GPSUTILS_CFLAGS) \
     $(LOCCORE_CFLAGS) \
     -I./ \
     -std=c++1y \
     -D__func__=__PRETTY_FUNCTION__ \
     -fno-short-enums

ACLOCAL_AMFLAGS = -I m4

requiredlibs = \
        $(GPSUTILS_LIBS) \
        $(LOCCORE_LIBS) \
        -llog

h_sources = \
    BatchingAdapter.h

libbatching_la_SOURCES = \
    location_batching.cpp \
    BatchingAdapter.cpp

if USE_GLIB
libbatching_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
#libbatching_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread $(requiredlibs) @GLIB_LIBS@ -shared -avoid-version
libbatching_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread $(requiredlibs) @GLIB_LIBS@ -avoid-version
libbatching_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libbatching_la_CFLAGS = $(AM_CFLAGS)
libbatching_la_LDFLAGS = -Wl,-z,defs -lpthread $(requiredlibs) -shared -version-info 1:0:0
libbatching_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

library_include_HEADERS = $(h_sources)

library_includedir = $(pkgincludedir)

#Create and Install libraries
lib_LTLIBRARIES = libbatching.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = location-batching.pc
sysconf_DATA = $(WORKSPACE)/hardware/qcom/gps/etc/flp.conf
EXTRA_DIST = $(pkgconfig_DATA)

