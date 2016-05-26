# function to find all *.cpp files under a directory
define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef


LOCAL_PATH:= $(call my-dir)
D_CFLAGS += -DNXP_LDR_SVC_VER_2=TRUE
######################################
# Build shared library system/lib/libp61-jcop-kit.so for stack code.

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm
LOCAL_MODULE := libp61-jcop-kit
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libhardware_legacy libcutils liblog libdl libhardware
LOCAL_CFLAGS := $(D_CFLAGS)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include/ \
    $(LOCAL_PATH)/inc/
LOCAL_SRC_FILES := \
    $(call all-c-files-under, src) \
    $(call all-cpp-files-under, src)

include $(BUILD_SHARED_LIBRARY)


######################################
include $(call all-makefiles-under,$(LOCAL_PATH))
