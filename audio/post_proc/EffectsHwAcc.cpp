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

#define LOG_TAG "EffectsHwAcc"
//#define LOG_NDEBUG 0

#include <utils/Log.h>
#include <media/EffectsFactoryApi.h>
#include <audio_effects/effect_hwaccelerator.h>
#include "EffectsHwAcc.h"

namespace android {

#define FRAME_SIZE(format)   ((format == AUDIO_FORMAT_PCM_24_BIT_PACKED) ? \
                              3 /* bytes for 24 bit */ : \
                              (format == AUDIO_FORMAT_PCM_16_BIT) ? \
                               sizeof(uint16_t) : sizeof(uint8_t))
// ----------------------------------------------------------------------------
EffectsHwAcc::EffectsBufferProvider::EffectsBufferProvider()
             : AudioBufferProvider(), mEffectsHandle(NULL),
               mInputBuffer(NULL), mOutputBuffer(NULL),
               mInputBufferFrameCountOffset(0)
{
}

EffectsHwAcc::EffectsBufferProvider::~EffectsBufferProvider()
{
    ALOGV(" deleting HwAccEffBufferProvider");

    if (mEffectsHandle)
        EffectRelease(mEffectsHandle);
    if (mInputBuffer)
        free(mInputBuffer);
    if (mOutputBuffer)
        free(mOutputBuffer);
}

status_t EffectsHwAcc::EffectsBufferProvider::getNextBuffer(
                       AudioBufferProvider::Buffer *pBuffer,
                       int64_t pts)
{
    ALOGV("EffectsBufferProvider::getNextBuffer");

    size_t reqInputFrameCount, frameCount, offset;
    size_t reqOutputFrameCount = pBuffer->frameCount;
    int ret = 0;

    if (mTrackInputBufferProvider != NULL) {
        while (1) {
            reqInputFrameCount = ((reqOutputFrameCount *
                                   mEffectsConfig.inputCfg.samplingRate)/
                                   mEffectsConfig.outputCfg.samplingRate) +
                                   (((reqOutputFrameCount *
                                     mEffectsConfig.inputCfg.samplingRate)%
                                     mEffectsConfig.outputCfg.samplingRate) ? 1 : 0);
            ALOGV("InputFrameCount: %d, OutputFrameCount: %d, InputBufferFrameCountOffset: %d",
                  reqInputFrameCount, reqOutputFrameCount,
                  mInputBufferFrameCountOffset);
            frameCount = reqInputFrameCount - mInputBufferFrameCountOffset;
            offset = mInputBufferFrameCountOffset *
                     FRAME_SIZE(mEffectsConfig.inputCfg.format) *
                     popcount(mEffectsConfig.inputCfg.channels);
            while (frameCount) {
                pBuffer->frameCount = frameCount;
                ret = mTrackInputBufferProvider->getNextBuffer(pBuffer, pts);
                if (ret == OK) {
                    int bytesInBuffer = pBuffer->frameCount *
                                        FRAME_SIZE(mEffectsConfig.inputCfg.format) *
                                        popcount(mEffectsConfig.inputCfg.channels);
                    memcpy((char *)mInputBuffer+offset, pBuffer->i8, bytesInBuffer);
                    frameCount -= pBuffer->frameCount;
                    mInputBufferFrameCountOffset += pBuffer->frameCount;
                    offset += bytesInBuffer;
                    mTrackInputBufferProvider->releaseBuffer(pBuffer);
                } else
                    break;
            }
            if (ret == OK) {
                mEffectsConfig.inputCfg.buffer.frameCount = reqInputFrameCount;
                mEffectsConfig.inputCfg.buffer.raw = (void *)mInputBuffer;
                mEffectsConfig.outputCfg.buffer.frameCount = reqOutputFrameCount;
                mEffectsConfig.outputCfg.buffer.raw = (void *)mOutputBuffer;

                ret = (*mEffectsHandle)->process(mEffectsHandle,
                                              &mEffectsConfig.inputCfg.buffer,
                                              &mEffectsConfig.outputCfg.buffer);
                if (ret == -ENODATA) {
                    ALOGV("Continue to provide more data for initial buffering");
                    mInputBufferFrameCountOffset -= reqInputFrameCount;
                    continue;
                }
                if (ret > 0)
                    mInputBufferFrameCountOffset -= reqInputFrameCount;
                pBuffer->raw = (void *)mOutputBuffer;
                pBuffer->frameCount = reqOutputFrameCount;
            }
            return ret;
        }
    } else {
        ALOGE("EffBufferProvider::getNextBuffer() error: NULL track buffer provider");
        return NO_INIT;
    }
}

void EffectsHwAcc::EffectsBufferProvider::releaseBuffer(
                                          AudioBufferProvider::Buffer *pBuffer)
{
    ALOGV("EffBufferProvider::releaseBuffer()");
    if (this->mTrackInputBufferProvider != NULL) {
        pBuffer->frameCount = 0;
        pBuffer->raw = NULL;
    } else {
        ALOGE("HwAccEffectsBufferProvider::releaseBuffer() error: NULL track buffer provider");
    }
}

EffectsHwAcc::EffectsHwAcc(uint32_t sampleRate)
             : mEnabled(false), mFd(-1), mBufferProvider(NULL),
               mInputSampleRate(sampleRate), mOutputSampleRate(sampleRate)
{
}

EffectsHwAcc::~EffectsHwAcc()
{
    ALOGV("deleting EffectsHwAcc");

    if (mBufferProvider)
        delete mBufferProvider;
}

void EffectsHwAcc::setSampleRate(uint32_t inpSR, uint32_t outSR)
{
    mInputSampleRate = inpSR;
    mOutputSampleRate = outSR;
}

void EffectsHwAcc::unprepareEffects(AudioBufferProvider **bufferProvider)
{
    ALOGV("EffectsHwAcc::unprepareEffects");

    EffectsBufferProvider *pHwAccbp = mBufferProvider;
    if (mBufferProvider != NULL) {
        ALOGV(" deleting h/w accelerator EffectsBufferProvider");
        int cmdStatus, status;
        uint32_t replySize = sizeof(int);

        replySize = sizeof(int);
        status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                              EFFECT_CMD_DISABLE,
                                              0 /*cmdSize*/, NULL /*pCmdData*/,
                                              &replySize, &cmdStatus /*pReplyData*/);
        if ((status != 0) || (cmdStatus != 0))
            ALOGE("error %d while enabling hw acc effects", status);

        *bufferProvider = pHwAccbp->mTrackBufferProvider;
        delete mBufferProvider;

        mBufferProvider = NULL;
    } else {
        ALOGV(" nothing to do, no h/w accelerator effects to delete");
    }
    mEnabled = false;
}

status_t EffectsHwAcc::prepareEffects(AudioBufferProvider **inputBufferProvider,
                                      AudioBufferProvider **bufferProvider,
                                      int sessionId,
                                      audio_channel_mask_t channelMask,
                                      int frameCount)
{
    ALOGV("EffectsHwAcc::prepareAccEffects");

    // discard the previous hw acc effects if there was one
    unprepareEffects(bufferProvider);

    EffectsBufferProvider* pHwAccbp = new EffectsBufferProvider();
    int32_t status;
    int cmdStatus;
    uint32_t replySize;
    uint32_t size = (sizeof(effect_param_t) + 2 * sizeof(int32_t) - 1) /
                    (sizeof(uint32_t) + 1);
    uint32_t buf32[size];
    effect_param_t *param = (effect_param_t *)buf32;

    uint32_t i, numEffects = 0;
    effect_descriptor_t hwAccFxDesc;
    int ret = EffectQueryNumberEffects(&numEffects);
    if (ret != 0) {
        ALOGE("AudioMixer() error %d querying number of effects", ret);
        goto noEffectsForActiveTrack;
    }
    ALOGV("EffectQueryNumberEffects() numEffects=%d", numEffects);

    for (i = 0 ; i < numEffects ; i++) {
        if (EffectQueryEffect(i, &hwAccFxDesc) == 0) {
            if (memcmp(&hwAccFxDesc.type, EFFECT_UIID_HWACCELERATOR,
                       sizeof(effect_uuid_t)) == 0) {
                ALOGI("found effect \"%s\" from %s",
                        hwAccFxDesc.name, hwAccFxDesc.implementor);
                break;
            }
        }
    }
    if (i == numEffects) {
        ALOGW("H/W accelerated effects library not found");
        goto noEffectsForActiveTrack;
    }
    if (EffectCreate(&hwAccFxDesc.uuid, sessionId, -1 /*ioId not relevant here*/,
                     &pHwAccbp->mEffectsHandle) != 0) {
        ALOGE("prepareEffects fails: error creating effect");
        goto noEffectsForActiveTrack;
    }

    // channel input configuration will be overridden per-track
    pHwAccbp->mEffectsConfig.inputCfg.channels = channelMask;
    pHwAccbp->mEffectsConfig.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    pHwAccbp->mEffectsConfig.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pHwAccbp->mEffectsConfig.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    pHwAccbp->mEffectsConfig.inputCfg.samplingRate = mInputSampleRate;
    pHwAccbp->mEffectsConfig.outputCfg.samplingRate = mOutputSampleRate;
    pHwAccbp->mEffectsConfig.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    pHwAccbp->mEffectsConfig.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_WRITE;
    pHwAccbp->mEffectsConfig.outputCfg.buffer.frameCount = frameCount;
    pHwAccbp->mEffectsConfig.inputCfg.mask = EFFECT_CONFIG_SMP_RATE | EFFECT_CONFIG_CHANNELS |
                                             EFFECT_CONFIG_FORMAT | EFFECT_CONFIG_ACC_MODE;
    pHwAccbp->mEffectsConfig.outputCfg.mask = pHwAccbp->mEffectsConfig.inputCfg.mask;

    // Configure hw acc effects
    replySize = sizeof(int);
    status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                          EFFECT_CMD_SET_CONFIG,
                                          sizeof(effect_config_t) /*cmdSize*/,
                                          &pHwAccbp->mEffectsConfig /*pCmdData*/,
                                          &replySize, &cmdStatus /*pReplyData*/);
    if ((status != 0) || (cmdStatus != 0)) {
        ALOGE("error %d while configuring h/w acc effects", status);
        goto noEffectsForActiveTrack;
    }
    replySize = sizeof(int);
    status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                          EFFECT_CMD_HW_ACC,
                                          sizeof(frameCount) /*cmdSize*/,
                                          &frameCount /*pCmdData*/,
                                          &replySize,
                                          &cmdStatus /*pReplyData*/);
    if ((status != 0) || (cmdStatus != 0)) {
        ALOGE("error %d while enabling h/w acc effects", status);
       goto noEffectsForActiveTrack;
    }
    replySize = sizeof(int);
    status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                          EFFECT_CMD_ENABLE,
                                          0 /*cmdSize*/, NULL /*pCmdData*/,
                                          &replySize, &cmdStatus /*pReplyData*/);
    if ((status != 0) || (cmdStatus != 0)) {
        ALOGE("error %d while enabling h/w acc effects", status);
        goto noEffectsForActiveTrack;
    }

    param->psize = sizeof(int32_t);
    *(int32_t *)param->data = HW_ACCELERATOR_FD;
    param->vsize = sizeof(int32_t);
    replySize = sizeof(effect_param_t) +
                ((param->psize - 1) / sizeof(int) + 1) * sizeof(int) +
                param->vsize;
    status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                          EFFECT_CMD_GET_PARAM,
                                          sizeof(effect_param_t) + param->psize,
                                          param, &replySize, param);
    if ((param->status != 0) || (*(int32_t *)(param->data + sizeof(int32_t)) <= 0)) {
            ALOGE("error %d while enabling h/w acc effects", status);
            goto noEffectsForActiveTrack;
    }
    mFd = *(int32_t *)(param->data + sizeof(int32_t));

    pHwAccbp->mInputBuffer = calloc(6*frameCount,
                                    /* 6 times buffering to account for an input of
                                       192kHz to an output of 32kHz - may be a least
                                       sampling rate of rendering device */
                                    FRAME_SIZE(pHwAccbp->mEffectsConfig.inputCfg.format) *
                                    popcount(channelMask));
    if (!pHwAccbp->mInputBuffer)
        goto noEffectsForActiveTrack;

    pHwAccbp->mOutputBuffer = calloc(frameCount,
                                     FRAME_SIZE(pHwAccbp->mEffectsConfig.outputCfg.format) *
                                     popcount(AUDIO_CHANNEL_OUT_STEREO));
    if (!pHwAccbp->mOutputBuffer) {
        free(pHwAccbp->mInputBuffer);
        goto noEffectsForActiveTrack;
    }
    // initialization successful:
    // - keep backup of track's buffer provider
    pHwAccbp->mTrackBufferProvider = *bufferProvider;
    pHwAccbp->mTrackInputBufferProvider = *inputBufferProvider;
    // - we'll use the hw acc effect integrated inside this track's buffer provider,
    //   and we'll use it as the track's buffer provider
    mBufferProvider = pHwAccbp;
    *bufferProvider = pHwAccbp;

    mEnabled = true;
    return NO_ERROR;

noEffectsForActiveTrack:
    delete pHwAccbp;
    mBufferProvider = NULL;
    return NO_INIT;
}

void EffectsHwAcc::setBufferProvider(AudioBufferProvider **trackInputBufferProvider,
                                     AudioBufferProvider **trackBufferProvider)
{
    ALOGV("setBufferProvider");
    if (mBufferProvider &&
        (mBufferProvider->mTrackInputBufferProvider != *trackInputBufferProvider)) {
        *trackBufferProvider = mBufferProvider;
        mBufferProvider->mTrackInputBufferProvider = *trackInputBufferProvider;
    }
}

#ifdef HW_ACC_HPX
void EffectsHwAcc::updateHPXState(uint32_t state)
{
    EffectsBufferProvider *pHwAccbp = mBufferProvider;
    if (pHwAccbp) {
        ALOGV("updateHPXState: %d", state);
        int cmdStatus, status;
        uint32_t replySize = sizeof(int);
        uint32_t data = state;
        uint32_t size = (sizeof(effect_param_t) + 2 * sizeof(int32_t));
        uint32_t buf32[size];
        effect_param_t *param = (effect_param_t *)buf32;

        param->psize = sizeof(int32_t);
        *(int32_t *)param->data = HW_ACCELERATOR_HPX_STATE;
        param->vsize = sizeof(int32_t);
        memcpy((param->data + param->psize), &data, param->vsize);
        status = (*pHwAccbp->mEffectsHandle)->command(pHwAccbp->mEffectsHandle,
                                          EFFECT_CMD_SET_PARAM,
                                          sizeof(effect_param_t) + param->psize +
                                          param->vsize,
                                          param, &replySize, &cmdStatus);

        if ((status != 0) || (cmdStatus != 0))
            ALOGE("error %d while updating HW ACC HPX BYPASS state", status);
    }
}
#endif
// ----------------------------------------------------------------------------
}; // namespace android
