#!/bin/bash

set -e

export DEVICE=oneplus3
export VENDOR=oneplus

MY_DIR=$(dirname "${BASH_SOURCE[0]}")
HELPER="$MY_DIR"/../../../vendor/cm/build/tools/extract_utils.sh
if [ ! -f "$HELPER" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
source "$HELPER"

if [ $# -eq 0 ]; then
  SRC=adb
else
  if [ $# -eq 1 ]; then
    SRC=$1
  else
    echo "$0: bad number of arguments"
    echo ""
    echo "usage: $0 [PATH_TO_EXPANDED_ROM]"
    echo ""
    echo "If PATH_TO_EXPANDED_ROM is not specified, blobs will be extracted from"
    echo "the device using adb pull."
    exit 1
  fi
fi

export SRC

BASE="$MY_DIR"/../../../vendor/$VENDOR/$DEVICE/proprietary
rm -rf "${BASE:?}/"*

extract "$MY_DIR"/proprietary-files-qc.txt "$BASE"
extract "$MY_DIR"/proprietary-files-qc-perf.txt "$BASE"
extract "$MY_DIR"/proprietary-files.txt "$BASE"

"$MY_DIR"/setup-makefiles.sh
