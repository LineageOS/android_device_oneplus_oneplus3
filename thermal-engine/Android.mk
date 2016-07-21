LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := thermal-engine
LOCAL_COPY_HEADERS := ./thermal_client.h

include $(BUILD_COPY_HEADERS)
