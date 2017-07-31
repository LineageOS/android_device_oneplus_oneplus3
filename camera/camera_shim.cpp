/*
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

extern "C" {

const char *_ZN7android18gClientPackageNameE;

// int32_t qcamera::QCameraParameters::setQuadraCfaMode(uint32_t enable, bool initCommit)
// _ZN7qcamera17QCameraParameters16setQuadraCfaModeEjb
int32_t _ZN7qcamera17QCameraParameters16setQuadraCfaModSHIM(uint32_t, bool)
{
    return 0;
}

// int32_t qcamera::QCameraParameters::setQuadraCfa(const QCameraParameters& params)
// _ZN7qcamera17QCameraParameters12setQuadraCfaERKS0_
int32_t _ZN7qcamera17QCameraParameters12setQuadraCfaERSHIM(const void *)
{
    return 0;
}

// bool qcamera::QCameraParameters::getQuadraCfa()
// _ZN7qcamera17QCameraParameters12getQuadraCfaEv
bool _ZN7qcamera17QCameraParameters12getQuadraCSHIM(void)
{
    return false;
}

// bool qcamera::isOneplusCamera(qcamera *this)
// _ZN7qcamera15isOneplusCameraEv
bool _ZN7qcamera15isOneplusCameSHIM(const void *)
{
    return true;
}

}
