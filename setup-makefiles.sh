#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

# Required!
export DEVICE=oneplus3
export VENDOR=oneplus

# Load extractutils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$MY_DIR" ]]; then MY_DIR="$PWD"; fi

export CM_ROOT="$MY_DIR"/../../..

HELPER="$CM_ROOT"/vendor/cm/build/tools/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

if [ -z "$PRODUCTMK" ]; then
    echo "Product makefile is not set!"
    exit 1
fi

if [ -z "$ANDROIDMK" ]; then
    echo "Android makefile is not set!"
    exit 1
fi

if [ -z "$BOARDMK" ]; then
    echo "Board makefile is not set!"
    exit 1
fi

# Copyright headers
write_header "$PRODUCTMK"
write_header "$ANDROIDMK"
write_header "$BOARDMK"

# The standard blobs
write_copy_list "$MY_DIR"/proprietary-files.txt
write_module_list "$MY_DIR"/proprietary-files.txt

# Qualcomm BSP blobs - we put a conditional around here
# in case the BSP is actually being built
echo "ifeq (\$(QCPATH),)" >> "$PRODUCTMK"
echo "ifeq (\$(QCPATH),)" >> "$ANDROIDMK"

write_copy_list "$MY_DIR"/proprietary-files-qc.txt
write_module_list "$MY_DIR"/proprietary-files-qc.txt

# Qualcomm performance blobs - conditional as well
# in order to support Cyanogen OS builds
cat << EOF >> "$PRODUCTMK"
endif

-include vendor/extra/devices.mk
ifneq (\$(call is-qc-perf-target),true)
EOF

cat << EOF >> "$ANDROIDMK"
endif

ifneq (\$(TARGET_HAVE_QC_PERF),true)
EOF

write_copy_list "$MY_DIR"/proprietary-files-qc-perf.txt
write_module_list "$MY_DIR"/proprietary-files-qc-perf.txt

echo "endif" >> "$PRODUCTMK"

cat << EOF >> "$ANDROIDMK"
endif

\$(shell mkdir -p \$(PRODUCT_OUT)/system/vendor/lib/egl && pushd \$(PRODUCT_OUT)/system/vendor/lib > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)
\$(shell mkdir -p \$(PRODUCT_OUT)/system/vendor/lib64/egl && pushd \$(PRODUCT_OUT)/system/vendor/lib64 > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)
EOF

