# ---------------------------------------------------------------------------------
#				MM-AUDIO-OSS-8K-AENC-QCELP13
# ---------------------------------------------------------------------------------

# cross-compiler flags
CFLAGS += -Wall
CFLAGS += -Wundef
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wno-trigraphs

# cross-compile flags specific to shared objects
CFLAGS_SO += -fpic

# required pre-processor flags
CPPFLAGS := -D__packed__=
CPPFLAGS += -DIMAGE_APPS_PROC
CPPFLAGS += -DFEATURE_Q_SINGLE_LINK
CPPFLAGS += -DFEATURE_Q_NO_SELF_QPTR
CPPFLAGS += -DFEATURE_LINUX
CPPFLAGS += -DFEATURE_NATIVELINUX
CPPFLAGS += -DFEATURE_DSM_DUP_ITEMS

CPPFLAGS += -g
CPPFALGS += -D_DEBUG
CPPFLAGS += -Iinc

# linker flags
LDFLAGS += -L$(SYSROOT)/usr/lib

# linker flags for shared objects
LDFLAGS_SO := -shared

# defintions
LIBMAJOR := $(basename $(basename $(LIBVER)))
LIBINSTALLDIR := $(DESTDIR)usr/lib
INCINSTALLDIR := $(DESTDIR)usr/include
BININSTALLDIR := $(DESTDIR)usr/bin

# ---------------------------------------------------------------------------------
#					BUILD
# ---------------------------------------------------------------------------------
all: libOmxQcelp13Enc.so.$(LIBVER) mm-aenc-omxqcelp13-test

install:
	echo "intalling aenc-qcelp13 in $(DESTDIR)"
	if [ ! -d $(LIBINSTALLDIR) ]; then mkdir -p $(LIBINSTALLDIR); fi
	if [ ! -d $(INCINSTALLDIR) ]; then mkdir -p $(INCINSTALLDIR); fi
	if [ ! -d $(BININSTALLDIR) ]; then mkdir -p $(BININSTALLDIR); fi
	install -m 555 libOmxQcelp13Enc.so.$(LIBVER) $(LIBINSTALLDIR)
	cd $(LIBINSTALLDIR) && ln -s libOmxQcelp13Enc.so.$(LIBVER) libOmxQcelp13Enc.so.$(LIBMAJOR)
	cd $(LIBINSTALLDIR) && ln -s libOmxQcelp13Enc.so.$(LIBMAJOR) libOmxQcelp13Enc.so
	install -m 555 mm-aenc-omxqcelp13-test $(BININSTALLDIR)
	
# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------
LDLIBS := -lpthread
LDLIBS += -lstdc++
LDLIBS += -lOmxCore

SRCS := src/omx_qcelp13_aenc.cpp
SRCS += src/aenc_svr.c

libOmxQcelp13Enc.so.$(LIBVER): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxQcelp13Enc.so.$(LIBMAJOR) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------
TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lOmxCore

TEST_SRCS := test/omx_qcelp13_enc_test.c

mm-aenc-omxqcelp13-test: libOmxQcelp13Enc.so.$(LIBVER) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
