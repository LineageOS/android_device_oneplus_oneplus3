/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define ATTRIBUTE_VALUE_DELIM ('=')
#define ATTRIBUTE_STRING_DELIM (";")

#define METADATA_PARSING_ERR (-1)
#define METADATA_PARSING_CONTINUE (0)
#define METADATA_PARSING_DONE (1)

#define MIN(x,y) (((x)>(y))?(y):(x))

struct video_encode_metadata_t {
    int hint_id;
    int state;
};

struct video_decode_metadata_t {
    int hint_id;
    int state;
};

struct audio_metadata_t {
    int hint_id;
    int state;
};

struct cam_preview_metadata_t {
    int hint_id;
    int state;
};

int parse_metadata(char *metadata, char **metadata_saveptr,
    char *attribute, int attribute_size, char *value,
    unsigned int value_size);
int parse_video_encode_metadata(char *metadata,
    struct video_encode_metadata_t *video_encode_metadata);
int parse_video_decode_metadata(char *metadata,
    struct video_decode_metadata_t *video_decode_metadata);
int parse_audio_metadata(char *metadata,
    struct audio_metadata_t *audio_metadata);
int parse_cam_preview_metadata(char *metadata,
    struct cam_preview_metadata_t *video_decode_metadata);
