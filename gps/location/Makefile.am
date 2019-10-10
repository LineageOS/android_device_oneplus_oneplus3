ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = \
     -I./ \
     -I../utils \
     $(LOCPLA_CFLAGS) \
     $(GPSUTILS_CFLAGS) \
     -std=c++11

liblocation_api_la_SOURCES = \
    LocationAPI.cpp \
    LocationAPIClientBase.cpp

if USE_GLIB
liblocation_api_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
liblocation_api_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
liblocation_api_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
liblocation_api_la_CFLAGS = $(AM_CFLAGS)
liblocation_api_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
liblocation_api_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

liblocation_api_la_LIBADD = -lstdc++ -ldl $(GPSUTILS_LIBS)

library_include_HEADERS = \
    LocationAPI.h \
    LocationAPIClientBase.h \
    location_interface.h \
    LocationDataTypes.h \
    ILocationAPI.h

#Create and Install libraries
lib_LTLIBRARIES = liblocation_api.la

library_includedir = $(pkgincludedir)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = location-api.pc
EXTRA_DIST = $(pkgconfig_DATA)
