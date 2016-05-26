EXTN_PN54X_PATH:= $(call my-dir)

LOCAL_C_INCLUDES += \
    $(EXTN_PN54X_PATH)/inc \
    $(EXTN_PN54X_PATH)/src/common \
    $(EXTN_PN54X_PATH)/src/log \
    $(EXTN_PN54X_PATH)/src/mifare \
    $(EXTN_PN54X_PATH)/src/utils

LOCAL_CFLAGS += -DNXP_UICC_ENABLE
