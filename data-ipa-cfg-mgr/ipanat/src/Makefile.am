AM_CFLAGS = -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs
if KERNELMODULES
AM_CFLAGS += -I./../inc $(KERNEL_DIR)/include
else
AM_CFLAGS += -I./../inc
endif
#AM_CFLAGS += -DDEBUG -g

common_CFLAGS =  -DUSE_GLIB @GLIB_CFLAGS@
if !KERNELMODULES
common_LDFLAGS = -lrt @GLIB_LIBS@
endif

if KERNELMODULES
library_includedir = ../inc $(KERNEL_DIR)/include $(pkgincludedir)
else
library_includedir = $(pkgincludedir)
endif

c_sources   = ipa_nat_drv.c \
              ipa_nat_drvi.c \
              ipa_nat_logi.c

library_include_HEADERS = ./../inc/ipa_nat_drvi.h \
                          ./../inc/ipa_nat_drv.h \
                          ./../inc/ipa_nat_logi.h

if KERNELMODULES
noinst_LIBRARIES = libipanat.a
libipanat_a_C = @C@
libipanat_a_SOURCES = $(c_sources)
else
lib_LTLIBRARIES = libipanat.la
libipanat_la_C = @C@
libipanat_la_SOURCES = $(c_sources)
libipanat_la_CFLAGS = $(AM_CFLAGS) $(common_CFLAGS)
libipanat_la_LDFLAGS = -shared $(common_LDFLAGS) -version-info 1:0:0
endif
