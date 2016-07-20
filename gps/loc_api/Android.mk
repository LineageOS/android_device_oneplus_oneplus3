LOCAL_PATH := $(call my-dir)

# add RPC dirs if RPC is available
ifneq ($(TARGET_NO_RPC),true)

GPS_DIR_LIST += $(LOCAL_PATH)/libloc_api-rpc-50001/

endif #TARGET_NO_RPC

GPS_DIR_LIST += $(LOCAL_PATH)/libloc_api_50001/

ifneq ($(QCPATH),)
GPS_DIR_LIST += \
    $(LOCAL_PATH)/ds_api/ \
    $(LOCAL_PATH)/loc_api_v02/
endif

#call the subfolders
include $(addsuffix Android.mk, $(GPS_DIR_LIST))
