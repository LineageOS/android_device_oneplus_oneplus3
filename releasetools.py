# Copyright (C) 2009 The Android Open Source Project
# Copyright (c) 2011, The Linux Foundation. All rights reserved.
# Copyright (C) 2017-2019 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import hashlib
import common
import re


def FullOTA_Assertions(info):
    AddModemAssertion(info, info.input_zip)


def IncrementalOTA_Assertions(info):
    AddModemAssertion(info, info.target_zip)


def AddModemAssertion(info, input_zip):
    android_info = input_zip.read("OTA/android-info.txt")
    m = re.search(r'require\s+version-modem\s*=\s*(.+)$', android_info)
    if m:
        modem_version, build_version = m.group(1).split('|')
        if modem_version and '*' not in modem_version:
            cmd = ('assert(op3.verify_modem("{}") == "1" || abort("Modem firmware '
                   'from {} or newer stock ROMs is prerequisite to be compatible '
                   'with this build."););'.format(modem_version, build_version))
            info.script.AppendExtra(cmd)
