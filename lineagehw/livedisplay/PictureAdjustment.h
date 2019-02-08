/*
 * Copyright (C) 2019 The LineageOS Project
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

#ifndef VENDOR_LINEAGE_LIVEDISPLAY_V2_0_PICTUREADJUSTMENT_H
#define VENDOR_LINEAGE_LIVEDISPLAY_V2_0_PICTUREADJUSTMENT_H

#include <vendor/lineage/livedisplay/2.0/IPictureAdjustment.h>

#include "SDMController.h"

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

using ::android::hardware::Return;
using ::android::hardware::Void;

class PictureAdjustment : public IPictureAdjustment {
   public:
    PictureAdjustment(std::shared_ptr<SDMController> controller, uint64_t cookie);

    bool isSupported();

    // Methods from ::vendor::lineage::livedisplay::V2_0::IPictureAdjustment follow.
    Return<void> getHueRange(getHueRange_cb _hidl_cb) override;
    Return<void> getSaturationRange(getSaturationRange_cb _hidl_cb) override;
    Return<void> getIntensityRange(getIntensityRange_cb _hidl_cb) override;
    Return<void> getContrastRange(getContrastRange_cb _hidl_cb) override;
    Return<void> getSaturationThresholdRange(getSaturationThresholdRange_cb _hidl_cb) override;
    Return<void> getPictureAdjustment(getPictureAdjustment_cb _hidl_cb) override;
    Return<void> getDefaultPictureAdjustment(getDefaultPictureAdjustment_cb _hidl_cb) override;
    Return<bool> setPictureAdjustment(
        const ::vendor::lineage::livedisplay::V2_0::HSIC& hsic) override;

    static void updateDefaultPictureAdjustment();

   private:
    std::shared_ptr<SDMController> mController;
    uint64_t mCookie;
    HSIC mDefaultPictureAdjustment;

    HSIC getPictureAdjustmentInternal();
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor

#endif  // VENDOR_LINEAGE_LIVEDISPLAY_V2_0_PICTUREADJUSTMENT_H
