LOCAL_PATH := $(call my-dir)
VOB_COMPONENTS := $(LOCAL_PATH)/../../libnfc-nci/src
NFA := $(VOB_COMPONENTS)/nfa
NFC := $(VOB_COMPONENTS)/nfc

include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))

LOCAL_PRELINK_MODULE := false

ifneq ($(NCI_VERSION),)
LOCAL_CFLAGS += -DNCI_VERSION=$(NCI_VERSION) -O0 -g
endif

LOCAL_CFLAGS += -Wall -Wextra
#variables for NFC_NXP_CHIP_TYPE
PN547C2 := 1
PN548C2 := 2
NQ110 := $PN547C2
NQ120 := $PN547C2
NQ210 := $PN548C2
NQ220 := $PN548C2

#NXP chip type Enable
ifeq ($(PN547C2),1)
LOCAL_CFLAGS += -DPN547C2=1
endif
ifeq ($(PN548C2),2)
LOCAL_CFLAGS += -DPN548C2=2
endif

#NXP PN547 Enable
LOCAL_CFLAGS += -DNXP_EXTNS=TRUE
LOCAL_CFLAGS += -DNFC_NXP_NON_STD_CARD=FALSE
LOCAL_CFLAGS += -DNFC_NXP_HFO_SETTINGS=FALSE

#### Select the JCOP OS Version ####
JCOP_VER_3_0 := 1
JCOP_VER_3_1_1 := 2
JCOP_VER_3_1_2 := 3
JCOP_VER_3_2 := 4
JCOP_VER_3_3 := 5

LOCAL_CFLAGS += -DJCOP_VER_3_0=$(JCOP_VER_3_0)
LOCAL_CFLAGS += -DJCOP_VER_3_1_1=$(JCOP_VER_3_1_1)
LOCAL_CFLAGS += -DJCOP_VER_3_1_2=$(JCOP_VER_3_1_2)
LOCAL_CFLAGS += -DJCOP_VER_3_2=$(JCOP_VER_3_2)
LOCAL_CFLAGS += -DJCOP_VER_3_3=$(JCOP_VER_3_3)

NFC_NXP_ESE:= TRUE
ifeq ($(NFC_NXP_ESE),TRUE)
LOCAL_CFLAGS += -DNFC_NXP_ESE=TRUE
LOCAL_CFLAGS += -DNFC_NXP_ESE_VER=$(JCOP_VER_3_3)
else
LOCAL_CFLAGS += -DNFC_NXP_ESE=FALSE
endif

#### Select the CHIP ####
NXP_CHIP_TYPE := $(PN548C2)

ifeq ($(NXP_CHIP_TYPE),$(PN547C2))
LOCAL_CFLAGS += -DNFC_NXP_CHIP_TYPE=PN547C2
else ifeq ($(NXP_CHIP_TYPE),$(PN548C2))
LOCAL_CFLAGS += -DNFC_NXP_CHIP_TYPE=PN548C2
endif

NFC_POWER_MANAGEMENT:= FALSE
ifeq ($(NFC_POWER_MANAGEMENT),TRUE)
LOCAL_CFLAGS += -DNFC_POWER_MANAGEMENT=TRUE
LOCAL_CFLAGS += -DNFC_NXP_TRIPLE_MODE_PROTECTION=TRUE
else
LOCAL_CFLAGS += -DNFC_POWER_MANAGEMENT=FALSE
endif

ifeq ($(NFC_NXP_ESE),TRUE)
LOCAL_CFLAGS += -DNXP_LDR_SVC_VER_2=TRUE
else
LOCAL_CFLAGS += -DNXP_LDR_SVC_VER_2=FALSE
endif


#Gemalto SE Support
LOCAL_CFLAGS += -DGEMATO_SE_SUPPORT
LOCAL_CFLAGS += -DNXP_UICC_ENABLE
define all-cpp-files-under
$(patsubst ./%,%, \
  $(shell cd $(LOCAL_PATH) ; \
          find $(1) -name "*.cpp" -and -not -name ".*") \
 )
endef

LOCAL_SRC_FILES += $(call all-cpp-files-under, .) $(call all-c-files-under, .)

LOCAL_C_INCLUDES += \
    frameworks/native/include \
    libcore/include \
    $(NFA)/include \
    $(NFA)/brcm \
    $(NFC)/include \
    $(NFC)/brcm \
    $(NFC)/int \
    $(VOB_COMPONENTS)/hal/include \
    $(VOB_COMPONENTS)/hal/int \
    $(VOB_COMPONENTS)/include \
    $(VOB_COMPONENTS)/gki/ulinux \
    $(VOB_COMPONENTS)/gki/common

ifeq ($(NFC_NXP_ESE),TRUE)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../p61-jcop-kit/include

endif

LOCAL_SHARED_LIBRARIES := \
    libicuuc \
    libnativehelper \
    libcutils \
    libutils \
    liblog \
    libnfc-nci

ifeq ($(NFC_NXP_ESE),TRUE)
LOCAL_SHARED_LIBRARIES += libp61-jcop-kit
endif

#LOCAL_STATIC_LIBRARIES := libxml2

LOCAL_MODULE := libnxp_nfc_nci_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
