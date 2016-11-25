# ---------------------------------------------------------------------------------
#				MM-AUDIO-OSS-8K-AENC-EVRC
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
all: libOmxEvrcEnc.so.$(LIBVER) mm-aenc-omxevrc-test

install:
	echo "intalling aenc-evrc in $(DESTDIR)"
	if [ ! -d $(LIBINSTALLDIR) ]; then mkdir -p $(LIBINSTALLDIR); fi
	if [ ! -d $(INCINSTALLDIR) ]; then mkdir -p $(INCINSTALLDIR); fi
	if [ ! -d $(BININSTALLDIR) ]; then mkdir -p $(BININSTALLDIR); fi
	install -m 555 libOmxEvrcEnc.so.$(LIBVER) $(LIBINSTALLDIR)
	cd $(LIBINSTALLDIR) && ln -s libOmxEvrcEnc.so.$(LIBVER) libOmxEvrcEnc.so.$(LIBMAJOR)
	cd $(LIBINSTALLDIR) && ln -s libOmxEvrcEnc.so.$(LIBMAJOR) libOmxEvrcEnc.so
	install -m 555 mm-aenc-omxevrc-test $(BININSTALLDIR)
	
# ---------------------------------------------------------------------------------
#				COMPILE LIBRARY
# ---------------------------------------------------------------------------------
LDLIBS := -lpthread
LDLIBS += -lstdc++
LDLIBS += -lOmxCore

SRCS := src/omx_evrc_aenc.cpp
SRCS += src/aenc_svr.c

libOmxEvrcEnc.so.$(LIBVER): $(SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS_SO) $(LDFLAGS_SO) -Wl,-soname,libOmxEvrcEnc.so.$(LIBMAJOR) -o $@ $^ $(LDFLAGS) $(LDLIBS)

# ---------------------------------------------------------------------------------
#				COMPILE TEST APP
# ---------------------------------------------------------------------------------
TEST_LDLIBS := -lpthread
TEST_LDLIBS += -ldl
TEST_LDLIBS += -lOmxCore

TEST_SRCS := test/omx_evrc_enc_test.c

mm-aenc-omxevrc-test: libOmxEvrcEnc.so.$(LIBVER) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(TEST_LDLIBS)

# ---------------------------------------------------------------------------------
#					END
# ---------------------------------------------------------------------------------
