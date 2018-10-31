LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := src/CtUpdateAmbassador.cpp \
                src/HAL.cpp \
                src/IpaEventRelay.cpp \
                src/LocalLogBuffer.cpp \
                src/OffloadStatistics.cpp \
                src/PrefixParser.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_MODULE := liboffloadhal

#LOCAL_CPP_FLAGS := -Wall -Werror
LOCAL_SHARED_LIBRARIES := libhwbinder \
                        libhidlbase \
                        libhidltransport \
                        liblog \
                        libcutils \
                        libdl \
                        libbase \
                        libutils \
                        libhardware_legacy \
                        libhardware \
                        android.hardware.tetheroffload.config@1.0 \
                        android.hardware.tetheroffload.control@1.0
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
include $(BUILD_SHARED_LIBRARY)
