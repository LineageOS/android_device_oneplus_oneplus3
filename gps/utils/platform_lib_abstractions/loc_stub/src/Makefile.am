AM_CFLAGS = \
     -I../include \
     -D__func__=__PRETTY_FUNCTION__ \
     -fno-short-enums

libloc_stub_la_extra_h = \
     ../include/utils/Log.h

libloc_stub_la_c_sources = \
     loc_stub_android_runtime.cpp \
     loc_stub_gettid.cpp \
     loc_stub_property_service.cpp \
     loc_stub_sched_policy.cpp \
     loc_stub_time.cpp

libloc_stub_la_SOURCES = $(libloc_stub_la_c_sources) $(libloc_stub_la_extra_h)

library_include_HEADERS = \
     ../include/loc_stub_android_runtime.h \
     ../include/loc_stub_gettid.h \
     ../include/loc_stub_property_service.h \
     ../include/loc_stub_sched_policy.h \
     ../include/loc_stub_time.h

library_includedir = $(pkgincludedir)

if USE_GLIB
libloc_stub_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_stub_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_stub_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_stub_la_CFLAGS = $(AM_CFLAGS)
libloc_stub_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libloc_stub_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_stub_la_LIBADD = -lstdc++ -ldl -llog

#Create and Install libraries
lib_LTLIBRARIES = libloc_stub.la
