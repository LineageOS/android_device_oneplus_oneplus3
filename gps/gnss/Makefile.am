AM_CFLAGS = \
     $(LOCPLA_CFLAGS) \
     $(LOCHAL_CFLAGS) \
     -I./ \
     -I../utils \
     -I../core \
     -I../location \
     -std=c++11

libgnss_la_SOURCES = \
    location_gnss.cpp \
    GnssAdapter.cpp \
    Agps.cpp

if USE_GLIB
libgnss_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgnss_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -avoid-version
libgnss_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgnss_la_CFLAGS = $(AM_CFLAGS)
libgnss_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libgnss_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libgnss_la_LIBADD = -lstdc++ $(LOCPLA_LIBS) $(LOCHAL_LIBS)


#Create and Install libraries
#lib_LTLIBRARIES = libgnss.la

#library_includedir = $(pkgincludedir)
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = location-api.pc
#EXTRA_DIST = $(pkgconfig_DATA)


libloc_ds_api_CFLAGS = \
     $(QMIF_CFLAGS) \
     $(QMI_CFLAGS) \
     $(DATA_CFLAGS) \
     $(GPSUTILS_CFLAGS) \
     -I$(WORKSPACE)/qcom-opensource/location/loc_api/ds_api

libloc_ds_api_la_SOURCES = \
     $(WORKSPACE)/qcom-opensource/location/loc_api/ds_api/ds_client.c

if USE_GLIB
libloc_ds_api_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_ds_api_CFLAGS) @GLIB_CFLAGS@
libloc_ds_api_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_ds_api_la_LDFLAGS += -Wl,--export-dynamic
libloc_ds_api_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_ds_api_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_ds_api_la_CFLAGS = $(AM_CFLAGS) $(libloc_ds_api_CFLAGS)
libloc_ds_api_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -Wl,--export-dynamic -shared -version-info 1:0:0
libloc_ds_api_la_LDFLAGS += -Wl,--export-dynamic
libloc_ds_api_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) $(libloc_ds_api_CFLAGS)
endif

libloc_ds_api_la_LIBADD = -lstdc++ $(QMIF_LIBS) -lqmiservices -ldsi_netctrl $(GPSUTILS_LIBS) $(LOCPLA_LIBS)

libloc_api_v02_CFLAGS = \
     $(QMIF_CFLAGS) \
     $(GPSUTILS_CFLAGS) \
     -I$(WORKSPACE)/qcom-opensource/location/loc_api/ds_api \
     -I$(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02

libloc_api_v02_la_SOURCES = \
    $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/LocApiV02.cpp \
    $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_log.c \
    $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_client.c \
    $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_sync_req.c \
    $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/location_service_v02.c

if USE_GLIB
libloc_api_v02_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_api_v02_CFLAGS) @GLIB_CFLAGS@
libloc_api_v02_la_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_api_v02_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_api_v02_la_CFLAGS = $(AM_CFLAGS) $(libloc_api_v02_CFLAGS)
libloc_api_v02_la_LDFLAGS = -lstdc++ -Wl,-z,defs -lpthread -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) $(libloc_api_v02_CFLAGS)
endif

libloc_api_v02_la_CXXFLAGS = -std=c++0x
libloc_api_v02_la_LIBADD = -lstdc++ -lqmi_cci -lqmi_common_so $(QMIF_LIBS) $(GPSUTILS_LIBS) $(LOCPLA_LIBS) ../core/libloc_core.la libloc_ds_api.la

library_include_HEADERS = \
   $(WORKSPACE)/qcom-opensource/location/loc_api/ds_api/ds_client.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/location_service_v02.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_log.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_client.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_sync_req.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/LocApiV02.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_util_log.h

library_includedir = $(pkgincludedir)

#Create and Install libraries
lib_LTLIBRARIES = libgnss.la libloc_ds_api.la libloc_api_v02.la
