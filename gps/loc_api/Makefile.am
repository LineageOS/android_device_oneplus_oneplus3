AM_CFLAGS = \
     -I./ \
     -I../core \
     -I./libloc_api_50001 \
     -I../utils \
     $(LOCPLA_CFLAGS) \
     -fno-short-enums \
     -D__func__=__PRETTY_FUNCTION__ \
     -DTARGET_USES_QCOM_BSP

libloc_eng_so_la_h_sources = \
     loc_eng_dmn_conn_glue_msg.h \
     loc_eng_dmn_conn_glue_pipe.h \
     loc_eng_dmn_conn.h \
     loc_eng_dmn_conn_handler.h \
     loc_eng_dmn_conn_thread_helper.h

libloc_eng_so_la_SOURCES = \
    libloc_api_50001/loc_eng.cpp \
    libloc_api_50001/loc_eng_agps.cpp \
    libloc_api_50001/loc_eng_xtra.cpp \
    libloc_api_50001/loc_eng_ni.cpp \
    libloc_api_50001/loc_eng_log.cpp \
    libloc_api_50001/loc_eng_nmea.cpp \
    libloc_api_50001/LocEngAdapter.cpp \
    libloc_api_50001/loc_eng_dmn_conn.cpp \
    libloc_api_50001/loc_eng_dmn_conn_handler.cpp \
    libloc_api_50001/loc_eng_dmn_conn_thread_helper.c \
    libloc_api_50001/loc_eng_dmn_conn_glue_msg.c \
    libloc_api_50001/loc_eng_dmn_conn_glue_pipe.c

libloc_eng_so_la_SOURCES += libloc_eng_so_la_h_sources

if USE_GLIB
libloc_eng_so_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libloc_eng_so_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_eng_so_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_eng_so_la_CFLAGS = $(AM_CFLAGS)
libloc_eng_so_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libloc_eng_so_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libloc_eng_so_la_LIBADD = -lstdc++ -ldl -llog $(LOCPLA_LIBS) ../utils/libgps_utils_so.la ../core/libloc_core.la


libgps_default_so_la_SOURCES = \
      libloc_api_50001/loc.cpp \
      libloc_api_50001/gps.c

if USE_GLIB
libgps_default_so_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgps_default_so_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libgps_default_so_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgps_default_so_la_CFLAGS = $(AM_CFLAGS)
libgps_default_so_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libgps_default_so_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libgps_default_so_la_LIBADD = -lstdc++ -llog $(LOCPLA_LIBS) ../utils/libgps_utils_so.la ../core/libloc_core.la -ldl libloc_eng_so.la


libloc_ds_api_CFLAGS = \
     $(QMIF_CFLAGS) \
     $(QMI_CFLAGS) \
     $(DATA_CFLAGS) \
     -I$(WORKSPACE)/qcom-opensource/location/loc_api/ds_api

libloc_ds_api_la_SOURCES = \
     $(WORKSPACE)/qcom-opensource/location/loc_api/ds_api/ds_client.c

if USE_GLIB
libloc_ds_api_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_ds_api_CFLAGS) @GLIB_CFLAGS@
libloc_ds_api_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_ds_api_la_LDFLAGS += -Wl,--export-dynamic
libloc_ds_api_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_ds_api_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_ds_api_la_CFLAGS = $(AM_CFLAGS) $(libloc_ds_api_CFLAGS)
libloc_ds_api_la_LDFLAGS = -lstdc++ -lpthread -Wl,--export-dynamic -shared -version-info 1:0:0
libloc_ds_api_la_LDFLAGS += -Wl,--export-dynamic
libloc_ds_api_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) $(libloc_ds_api_CFLAGS)
endif

libloc_ds_api_la_LIBADD = -lstdc++ $(QMIF_LIBS) -lqmiservices -ldsi_netctrl $(LOCPLA_LIBS) ../utils/libgps_utils_so.la
#libloc_ds_api_la_LIBADD = -lstdc++ $(QMIF_LIBS) -lqmiservices $(LOCPLA_LIBS) ../utils/libgps_utils_so.la

libloc_api_v02_CFLAGS = \
     $(QMIF_CFLAGS) \
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
libloc_api_v02_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(libloc_api_v02_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libloc_api_v02_la_CFLAGS = $(AM_CFLAGS) $(libloc_api_v02_CFLAGS)
libloc_api_v02_la_LDFLAGS = -lstdc++ -lpthread -shared -version-info 1:0:0
libloc_api_v02_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) $(libloc_api_v02_CFLAGS)
endif

libloc_api_v02_la_CXXFLAGS = -std=c++0x
libloc_api_v02_la_LIBADD = -lstdc++ -lqmi_cci -lqmi_common_so $(QMIF_LIBS) $(LOCPLA_LIBS) ../core/libloc_core.la ../utils/libgps_utils_so.la libloc_ds_api.la

library_include_HEADERS = \
   libloc_api_50001/LocEngAdapter.h \
   libloc_api_50001/loc.h \
   libloc_api_50001/loc_eng.h \
   libloc_api_50001/loc_eng_xtra.h \
   libloc_api_50001/loc_eng_ni.h \
   libloc_api_50001/loc_eng_agps.h \
   libloc_api_50001/loc_eng_msg.h \
   libloc_api_50001/loc_eng_log.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/ds_api/ds_client.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/location_service_v02.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_log.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_v02_client.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_api_sync_req.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/LocApiV02.h \
   $(WORKSPACE)/qcom-opensource/location/loc_api/loc_api_v02/loc_util_log.h

library_includedir = $(pkgincludedir)

#Create and Install libraries
lib_LTLIBRARIES = libloc_eng_so.la libgps_default_so.la libloc_ds_api.la libloc_api_v02.la
