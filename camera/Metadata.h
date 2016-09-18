/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef METADATA_H_
#define METADATA_H_

#include <stdint.h>
#include <hardware/camera3.h>
#include <system/camera_metadata.h>

// Metadata is a convenience class for dealing with libcamera_metadata
class Metadata {
    public:
        Metadata();
        ~Metadata();
        // Initialize with framework metadata
        int init(const camera_metadata_t *metadata);

        // Parse and add an entry. Allocates and copies new storage for *data.
        int addUInt8(uint32_t tag, int count, const uint8_t *data);
        int add1UInt8(uint32_t tag, const uint8_t data);
        int addInt32(uint32_t tag, int count, const int32_t *data);
        int addFloat(uint32_t tag, int count, const float *data);
        int addInt64(uint32_t tag, int count, const int64_t *data);
        int addDouble(uint32_t tag, int count, const double *data);
        int addRational(uint32_t tag, int count,
                const camera_metadata_rational_t *data);

        // Get a handle to the current metadata
        // This is not a durable handle, and may be destroyed by add*/init
        camera_metadata_t* get();

    private:
        // Actual internal storage
        camera_metadata_t* mData;
        // Destroy old metadata and replace with new
        void replace(camera_metadata_t *m);
        // Validate the tag, type and count for a metadata entry
        bool validate(uint32_t tag, int tag_type, int count);
        // Add a verified tag with data
        int add(uint32_t tag, int count, const void *tag_data);
};

#endif // METADATA_H_
