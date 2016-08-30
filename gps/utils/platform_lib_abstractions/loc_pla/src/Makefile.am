AM_CFLAGS = \
     $(LOCSTUB_CFLAGS) \
     -I../include \
     -D__func__=__PRETTY_FUNCTION__ \
     -fno-short-enums

h_sources = \
    ../include/platform_lib_android_runtime.h \
    ../include/platform_lib_gettid.h \
    ../include/platform_lib_includes.h \
    ../include/platform_lib_log_util.h \
    ../include/platform_lib_macros.h \
    ../include/platform_lib_property_service.h \
    ../include/platform_lib_sched_policy.h \
    ../include/platform_lib_time.h

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)

libloc_pla_la_SOURCES = \
    platform_lib_android_runtime.cpp \
    platform_lib_gettid.cpp \
    platform_lib_log_util.cpp \
    platform_lib_property_service.cpp \
    platform_lib_sched_policy.cpp \
    platform_lib_time.cpp

if USE_GLIB
libloc_pla_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_pla_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_pla_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_pla_la_CFLAGS = $(AM_CFLAGS)
libloc_pla_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libloc_pla_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_pla_la_LIBADD = -lstdc++ -ldl -llog $(LOCSTUB_LIBS)

#Create and Install libraries
lib_LTLIBRARIES = libloc_pla.la
