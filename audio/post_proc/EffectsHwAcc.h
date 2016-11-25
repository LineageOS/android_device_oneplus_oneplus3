/*
 * Copyright (c) 2014-15, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
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
 */

#ifndef ANDROID_EFFECTS_HW_ACC_H
#define ANDROID_EFFECTS_HW_ACC_H

#include <media/AudioBufferProvider.h>

namespace android {

// ----------------------------------------------------------------------------
class EffectsHwAcc {
public:
    EffectsHwAcc(uint32_t sampleRate);
    virtual ~EffectsHwAcc();

    virtual void setSampleRate(uint32_t inpSR, uint32_t outSR);
    virtual void unprepareEffects(AudioBufferProvider **trackBufferProvider);
    virtual status_t prepareEffects(AudioBufferProvider **trackInputBufferProvider,
                            AudioBufferProvider **trackBufferProvider,
                            int sessionId, audio_channel_mask_t channelMask,
                            int frameCount);
    virtual void setBufferProvider(AudioBufferProvider **trackInputbufferProvider,
                           AudioBufferProvider **trackBufferProvider);
#ifdef HW_ACC_HPX
    virtual void updateHPXState(uint32_t state);
#endif

    /* AudioBufferProvider that wraps a track AudioBufferProvider by a call to
       h/w accelerated effect */
    class EffectsBufferProvider : public AudioBufferProvider {
    public:
        EffectsBufferProvider();
        virtual ~EffectsBufferProvider();

        virtual status_t getNextBuffer(Buffer* buffer, int64_t pts);
        virtual void releaseBuffer(Buffer* buffer);

        AudioBufferProvider* mTrackInputBufferProvider;
        AudioBufferProvider* mTrackBufferProvider;
        effect_handle_t    mEffectsHandle;
        effect_config_t    mEffectsConfig;

        void *mInputBuffer;
        void *mOutputBuffer;
        uint32_t mInputBufferFrameCountOffset;
    };

    bool mEnabled;
    int32_t mFd;

    EffectsBufferProvider* mBufferProvider;

private:
    uint32_t mInputSampleRate;
    uint32_t mOutputSampleRate;
};


// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_EFFECTS_HW_ACC_H
