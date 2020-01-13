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

#define LOG_TAG "vendor.lineage.livedisplay@2.0-impl.oneplus3"

#include "SDMController.h"

#include <android-base/logging.h>
#include <dlfcn.h>

#define LOAD_SDM_FUNCTION(name) \
    fn_##name##_ = loadFunction<disp_api_##name>(handle_.get(), "disp_api_" #name);

#define CLOSE_SDM_FUNCTION(name) fn_##name##_ = nullptr;

#define FOR_EACH_FUNCTION(MACRO)    \
    MACRO(init)                     \
    MACRO(deinit)                   \
    MACRO(get_num_display_modes)    \
    MACRO(get_display_modes)        \
    MACRO(get_active_display_mode)  \
    MACRO(set_active_display_mode)  \
    MACRO(set_default_display_mode) \
    MACRO(get_default_display_mode) \
    MACRO(get_global_pa_range)      \
    MACRO(get_global_pa_config)     \
    MACRO(set_global_pa_config)     \
    MACRO(get_feature_version)

#define CONTROLLER_CHECK(function, ...)      \
    if (fn_##function##_ == nullptr) {       \
        return -1;                           \
    }                                        \
    int err = fn_##function##_(__VA_ARGS__); \
    if (err != 0) {                          \
        return err;                          \
    }                                        \
    return 0;

namespace {
#ifdef LIVES_IN_SYSTEM
constexpr char kFilename[] = "libsdm-disp-apis.so";
#else
constexpr char kFilename[] = "libsdm-disp-vndapis.so";
#endif

template <typename Function>
Function loadFunction(void* handle, const char* name) {
    void* fn = dlsym(handle, name);
    if (fn == nullptr) {
        LOG(ERROR) << "loadFunction -- failed to load function " << name;
    }
    return reinterpret_cast<Function>(fn);
}
}  // anonymous namespace

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_0 {
namespace sdm {

SDMController::SDMController()
    : handle_(dlopen(kFilename, RTLD_NOW), [this](void* p) {
          FOR_EACH_FUNCTION(CLOSE_SDM_FUNCTION)
          if (p != nullptr) {
              int err = dlclose(p);
              p = nullptr;
              if (err != 0) {
                  LOG(ERROR) << "DLCLOSE failed for " << kFilename;
              }
          }
      }) {
    if (handle_ == nullptr) {
        LOG(FATAL) << "DLOPEN failed for " << kFilename << " (" << dlerror() << ")";
    }

    FOR_EACH_FUNCTION(LOAD_SDM_FUNCTION)

    // Initialize SDM backend
    uint64_t hctx = 0;
    if (init(&hctx, 0) == 0) {
        hctx_ = hctx;
    } else {
        LOG(FATAL) << "Failed to initialize SDM backend";
    }
}

SDMController::~SDMController() {
    deinit(hctx_, 0);
}

int32_t SDMController::init(uint64_t* hctx, uint32_t flags) {
    CONTROLLER_CHECK(init, hctx, flags);
}

int32_t SDMController::deinit(uint64_t hctx, uint32_t flags) {
    CONTROLLER_CHECK(deinit, hctx, flags);
}

int32_t SDMController::getNumDisplayModes(uint32_t disp_id, uint32_t mode_type, int32_t* mode_cnt,
                                          uint32_t* flags) {
    CONTROLLER_CHECK(get_num_display_modes, hctx_, disp_id, mode_type, mode_cnt, flags);
}

int32_t SDMController::getDisplayModes(uint32_t disp_id, uint32_t mode_type, void* modes,
                                       int32_t mode_cnt, uint32_t* flags) {
    CONTROLLER_CHECK(get_display_modes, hctx_, disp_id, mode_type, modes, mode_cnt, flags);
}

int32_t SDMController::getActiveDisplayMode(uint32_t disp_id, int32_t* mode_id, uint32_t* mask,
                                            uint32_t* flags) {
    CONTROLLER_CHECK(get_active_display_mode, hctx_, disp_id, mode_id, mask, flags);
}

int32_t SDMController::setActiveDisplayMode(uint32_t disp_id, int32_t mode_id, uint32_t flags) {
    CONTROLLER_CHECK(set_active_display_mode, hctx_, disp_id, mode_id, flags);
}

int32_t SDMController::setDefaultDisplayMode(uint32_t disp_id, int32_t mode_id, uint32_t flags) {
    CONTROLLER_CHECK(set_default_display_mode, hctx_, disp_id, mode_id, flags);
}

int32_t SDMController::getDefaultDisplayMode(uint32_t disp_id, int32_t* mode_id, uint32_t* flags) {
    CONTROLLER_CHECK(get_default_display_mode, hctx_, disp_id, mode_id, flags);
}

int32_t SDMController::getGlobalPaRange(uint32_t disp_id, void* range) {
    CONTROLLER_CHECK(get_global_pa_range, hctx_, disp_id, range);
}

int32_t SDMController::getGlobalPaConfig(uint32_t disp_id, uint32_t* enable, void* cfg) {
    CONTROLLER_CHECK(get_global_pa_config, hctx_, disp_id, enable, cfg);
}

int32_t SDMController::setGlobalPaConfig(uint32_t disp_id, uint32_t enable, void* cfg) {
    CONTROLLER_CHECK(set_global_pa_config, hctx_, disp_id, enable, cfg);
}

int32_t SDMController::getFeatureVersion(uint32_t feature_id, void* ver, uint32_t* flags) {
    CONTROLLER_CHECK(get_feature_version, hctx_, feature_id, ver, flags);
}

}  // namespace sdm
}  // namespace V2_0
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
