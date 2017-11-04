AM_CFLAGS = -Wundef \
         -I./ \
         -std=c++11 \
         $(LOCPLA_CFLAGS)

AM_CPPFLAGS = -Wundef \
         -I./ \
         -std=c++11 \
         $(LOCPLA_CFLAGS)

libgps_utils_so_la_h_sources = \
        msg_q.h \
        linked_list.h \
        loc_cfg.h \
        loc_log.h \
        loc_target.h \
        loc_timer.h \
        MsgTask.h \
        LocHeap.h \
        LocThread.h \
        LocTimer.h \
        loc_misc_utils.h \
        loc_nmea.h \
        gps_extended_c.h \
        gps_extended.h \
        loc_gps.h

libgps_utils_so_la_c_sources = \
        linked_list.c \
        msg_q.c \
        loc_cfg.cpp \
        loc_log.cpp \
        loc_target.cpp \
        LocHeap.cpp \
        LocTimer.cpp \
        LocThread.cpp \
        MsgTask.cpp \
        loc_misc_utils.cpp \
        loc_nmea.cpp


library_include_HEADERS = $(libgps_utils_so_la_h_sources)

libgps_utils_so_la_SOURCES = $(libgps_utils_so_la_c_sources)

if USE_GLIB
libgps_utils_so_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgps_utils_so_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libgps_utils_so_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgps_utils_so_la_CFLAGS = $(AM_CFLAGS)
libgps_utils_so_la_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libgps_utils_so_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libgps_utils_so_la_LIBADD = -lcutils -lstdc++ -llog $(LOCPLA_LIBS)

#Create and Install libraries
lib_LTLIBRARIES = libgps_utils_so.la
library_includedir = $(pkgincludedir)
pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = gps-utils.pc
EXTRA_DIST = $(pkgconfig_DATA)
