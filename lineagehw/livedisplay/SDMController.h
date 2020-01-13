/*
 * Copyright (C) 2018-2020 The LineageOS Project
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

#pragma once

#include <android-base/macros.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

class SDMController {
  public:
    SDMController();
    ~SDMController();

    int32_t getNumDisplayModes(uint32_t disp_id, uint32_t mode_type, int32_t* mode_cnt,
                               uint32_t* flags);
    int32_t getDisplayModes(uint32_t disp_id, uint32_t mode_type, void* modes, int32_t mode_cnt,
                            uint32_t* flags);
    int32_t getActiveDisplayMode(uint32_t disp_id, int32_t* mode_id, uint32_t* mask,
                                 uint32_t* flags);
    int32_t setActiveDisplayMode(uint32_t disp_id, int32_t mode_id, uint32_t flags);
    int32_t setDefaultDisplayMode(uint32_t disp_id, int32_t mode_id, uint32_t flags);
    int32_t getDefaultDisplayMode(uint32_t disp_id, int32_t* mode_id, uint32_t* flags);
    int32_t getGlobalPaRange(uint32_t disp_id, void* range);
    int32_t getGlobalPaConfig(uint32_t disp_id, uint32_t* enable, void* cfg);
    int32_t setGlobalPaConfig(uint32_t disp_id, uint32_t enable, void* cfg);
    int32_t getFeatureVersion(uint32_t feature_id, void* ver, uint32_t* flags);

  private:
    int32_t init(uint64_t* hctx, uint32_t flags);
    int32_t deinit(uint64_t hctx, uint32_t flags);

    typedef int32_t (*disp_api_init)(uint64_t*, uint32_t);
    typedef int32_t (*disp_api_deinit)(uint64_t, uint32_t);
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

    std::unique_ptr<void, std::function<void(void*)>> handle_;
    uint64_t hctx_;

    disp_api_init fn_init_;
    disp_api_deinit fn_deinit_;
    disp_api_get_num_display_modes fn_get_num_display_modes_;
    disp_api_get_display_modes fn_get_display_modes_;
    disp_api_get_active_display_mode fn_get_active_display_mode_;
    disp_api_set_active_display_mode fn_set_active_display_mode_;
    disp_api_set_default_display_mode fn_set_default_display_mode_;
    disp_api_get_default_display_mode fn_get_default_display_mode_;
    disp_api_get_global_pa_range fn_get_global_pa_range_;
    disp_api_get_global_pa_config fn_get_global_pa_config_;
    disp_api_set_global_pa_config fn_set_global_pa_config_;
    disp_api_get_feature_version fn_get_feature_version_;

    DISALLOW_COPY_AND_ASSIGN(SDMController);
};

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
