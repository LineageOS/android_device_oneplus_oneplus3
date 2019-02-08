/*
 * Copyright (C) 2018-2019 The LineageOS Project
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

#ifndef VENDOR_LINEAGE_LIVEDISPLAY_V2_0_SDMCONTROLLER_H
#define VENDOR_LINEAGE_LIVEDISPLAY_V2_0_SDMCONTROLLER_H

#include <memory>

#include <stdint.h>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

class SDMController {
   public:
    SDMController();

    int32_t init(uint64_t* hctx, uint32_t flags);
    int32_t deinit(uint64_t hctx, uint32_t flags);
    int32_t get_global_color_balance_range(uint64_t hctx, uint32_t disp_id, void* range);
    int32_t set_global_color_balance(uint64_t hctx, uint32_t disp_id, int32_t warmness,
                                     uint32_t flags);
    int32_t get_global_color_balance(uint64_t hctx, uint32_t disp_id, int32_t* warmness,
                                     uint32_t* flags);
    int32_t get_num_display_modes(uint64_t hctx, uint32_t disp_id, uint32_t mode_type,
                                  int32_t* mode_cnt, uint32_t* flags);
    int32_t get_display_modes(uint64_t hctx, uint32_t disp_id, uint32_t mode_type, void* modes,
                              int32_t mode_cnt, uint32_t* flags);
    int32_t get_active_display_mode(uint64_t hctx, uint32_t disp_id, int32_t* mode_id,
                                    uint32_t* mask, uint32_t* flags);
    int32_t set_active_display_mode(uint64_t hctx, uint32_t disp_id, int32_t mode_id,
                                    uint32_t flags);
    int32_t set_default_display_mode(uint64_t hctx, uint32_t disp_id, int32_t mode_id,
                                     uint32_t flags);
    int32_t get_default_display_mode(uint64_t hctx, uint32_t disp_id, int32_t* mode_id,
                                     uint32_t* flags);
    int32_t get_global_pa_range(uint64_t hctx, uint32_t disp_id, void* range);
    int32_t get_global_pa_config(uint64_t hctx, uint32_t disp_id, uint32_t* enable, void* cfg);
    int32_t set_global_pa_config(uint64_t hctx, uint32_t disp_id, uint32_t enable, void* cfg);
    int32_t get_feature_version(uint64_t hctx, uint32_t feature_id, void* ver, uint32_t* flags);

   private:
    typedef int32_t (*disp_api_init)(uint64_t*, uint32_t);
    typedef int32_t (*disp_api_deinit)(uint64_t, uint32_t);
    typedef int32_t (*disp_api_get_global_color_balance_range)(uint64_t, uint32_t, void*);
    typedef int32_t (*disp_api_set_global_color_balance)(uint64_t, uint32_t, int32_t, uint32_t);
    typedef int32_t (*disp_api_get_global_color_balance)(uint64_t, uint32_t, int32_t*, uint32_t*);
    typedef int32_t (*disp_api_get_num_display_modes)(uint64_t, uint32_t, int32_t, int32_t*,
                                                      uint32_t*);
    typedef int32_t (*disp_api_get_display_modes)(uint64_t, uint32_t, int32_t, void*, int32_t,
                                                  uint32_t*);
    typedef int32_t (*disp_api_get_active_display_mode)(uint64_t, uint32_t, int32_t*, uint32_t*,
                                                        uint32_t*);
    typedef int32_t (*disp_api_set_active_display_mode)(uint64_t, uint32_t, int32_t, uint32_t);
    typedef int32_t (*disp_api_set_default_display_mode)(uint64_t, uint32_t, int32_t, uint32_t);
    typedef int32_t (*disp_api_get_default_display_mode)(uint64_t, uint32_t, int32_t*, uint32_t*);
    typedef int32_t (*disp_api_get_global_pa_range)(uint64_t, uint32_t, void*);
    typedef int32_t (*disp_api_get_global_pa_config)(uint64_t, uint32_t, uint32_t*, void*);
    typedef int32_t (*disp_api_set_global_pa_config)(uint64_t, uint32_t, uint32_t, void*);
    typedef int32_t (*disp_api_get_feature_version)(uint64_t, uint32_t, void*, uint32_t*);

    std::shared_ptr<void> mHandle;
    disp_api_init mFn_init;
    disp_api_deinit mFn_deinit;
    disp_api_get_global_color_balance_range mFn_get_global_color_balance_range;
    disp_api_set_global_color_balance mFn_set_global_color_balance;
    disp_api_get_global_color_balance mFn_get_global_color_balance;
    disp_api_get_num_display_modes mFn_get_num_display_modes;
    disp_api_get_display_modes mFn_get_display_modes;
    disp_api_get_active_display_mode mFn_get_active_display_mode;
    disp_api_set_active_display_mode mFn_set_active_display_mode;
    disp_api_set_default_display_mode mFn_set_default_display_mode;
    disp_api_get_default_display_mode mFn_get_default_display_mode;
    disp_api_get_global_pa_range mFn_get_global_pa_range;
    disp_api_get_global_pa_config mFn_get_global_pa_config;
    disp_api_set_global_pa_config mFn_set_global_pa_config;
    disp_api_get_feature_version mFn_get_feature_version;
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor

#endif  // VENDOR_LINEAGE_LIVEDISPLAY_V2_0_SDMCONTROLLER_H
