#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
#           (C) 2017 The LineageOS Project
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
DEVICE=oneplus3
VENDOR=oneplus

INITIAL_COPYRIGHT_YEAR=2016

# Load extractutils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$MY_DIR" ]]; then MY_DIR="$PWD"; fi

LINEAGE_ROOT="$MY_DIR"/../../..

HELPER="$LINEAGE_ROOT"/vendor/lineage/build/tools/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
. "$HELPER"

# Initialize the helper
setup_vendor "$DEVICE" "$VENDOR" "$LINEAGE_ROOT"

# Copyright headers and guards
write_headers

# The standard blobs
write_makefiles "$MY_DIR"/proprietary-files.txt true

# Qualcomm BSP blobs - we put a conditional around here
# in case the BSP is actually being built
printf '\n%s\n' "ifeq (\$(QCPATH),)" >> "$PRODUCTMK"
printf '\n%s\n' "ifeq (\$(QCPATH),)" >> "$ANDROIDMK"

write_makefiles "$MY_DIR"/proprietary-files-qc.txt true

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

write_makefiles "$MY_DIR"/proprietary-files-qc-perf.txt true

echo "endif" >> "$PRODUCTMK"

cat << EOF >> "$ANDROIDMK"

endif

\$(shell mkdir -p \$(PRODUCT_OUT)/system/vendor/lib/egl && pushd \$(PRODUCT_OUT)/system/vendor/lib > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)
\$(shell mkdir -p \$(PRODUCT_OUT)/system/vendor/lib64/egl && pushd \$(PRODUCT_OUT)/system/vendor/lib64 > /dev/null && ln -s egl/libEGL_adreno.so libEGL_adreno.so && popd > /dev/null)
EOF

# We are done!
write_footers
