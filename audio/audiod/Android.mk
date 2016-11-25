LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifneq (,$(findstring $(PLATFORM_VERSION), 5.0 5.1 5.1.1))
include external/stlport/libstlport.mk
endif

LOCAL_SRC_FILES:= \
	audiod_main.cpp \
	AudioDaemon.cpp \

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libmedia

ifneq (,$(findstring $(PLATFORM_VERSION), 5.0 5.1 5.1.1))
LOCAL_SHARED_LIBRARIES += libstlport
endif

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_MODULE:= audiod

include $(BUILD_EXECUTABLE)
