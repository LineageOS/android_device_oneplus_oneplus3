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

#include <cutils/properties.h>
#include <stdint.h>
#include <string.h>

extern "C" {

static bool is_op3t_front_camera;

const char *_ZN7android18gClientPackageNameE;

// int32_t qcamera::QCameraParameters::setQuadraCfaMode(qcamera::QCameraParameters *this,
//         uint32_t enable, bool initCommit)
// _ZN7qcamera17QCameraParameters16setQuadraCfaModeEjb
int32_t _ZN7qcamera17QCameraParameters16setQuadraCfaModSHIM(const void *, uint32_t, bool)
{
    return 0;
}

// int32_t qcamera::QCameraParameters::setQuadraCfa(qcamera::QCameraParameters *this,
//         const QCameraParameters *params)
// _ZN7qcamera17QCameraParameters12setQuadraCfaERKS0_
int32_t _ZN7qcamera17QCameraParameters12setQuadraCfaERSHIM(const void *ptr, const void *)
{
    char prop[PROPERTY_VALUE_MAX];

    // These hacks break OnePlus3's front camera; only use them on OnePlus3T
    if (!property_get("ro.product.device", prop, NULL) ||
            strcmp(prop, "OnePlus3T"))
        return 0;

    // is_op3t_front_camera = this->m_pCapability->supported_focus_modes_cnt == 1;
    // This uses supported_focus_modes_cnt to determine whether the front or rear
    // camera is in use (front camera only has one focus mode, so supported_focus_modes_cnt
    // is one). This is needed because when some of the functions here return true, photo
    // capture fails with the rear camera. Abuse this function to set is_op3t_front_camera since
    // it's regularly called from QCameraParameters::updateParameters().
    is_op3t_front_camera = *((uint32_t *)(*((uint32_t *)ptr + 112) + 1304)) == 1;
    return 0;
}

// bool qcamera::QCameraParameters::getQuadraCfa(qcamera::QCameraParameters *this)
// _ZN7qcamera17QCameraParameters12getQuadraCfaEv
bool _ZN7qcamera17QCameraParameters12getQuadraCSHIM(const void *)
{
    return false;
}

// bool qcamera::isOneplusCamera(qcamera *this)
// _ZN7qcamera15isOneplusCameraEv
bool _ZN7qcamera15isOneplusCameSHIM(const void *)
{
    return true;
}

// bool qcamera::QCameraParameters::is3p8spLowLight(qcamera::QCameraParameters *this)
// _ZN7qcamera17QCameraParameters15is3p8spLowLightEv
bool _ZN7qcamera17QCameraParameters15is3p8spLowLigSHIM(const void *)
{
    return is_op3t_front_camera;
}

// bool qcamera::QCameraParameters::handleSuperResoultion(qcamera::QCameraParameters *this)
// _ZN7qcamera17QCameraParameters21handleSuperResoultionEv
bool _ZN7qcamera17QCameraParameters21handleSuperResoultiSHIM(const void *ptr)
{
    *((uint8_t *)ptr + 2620) = is_op3t_front_camera;
    return is_op3t_front_camera;
}

// bool qcamera::QCameraParameters::isSuperResoultion(qcamera::QCameraParameters *this)
// _ZN7qcamera17QCameraParameters17isSuperResoultionEv
bool _ZN7qcamera17QCameraParameters17isSuperResoultiSHIM(const void *)
{
    return is_op3t_front_camera;
}

}
