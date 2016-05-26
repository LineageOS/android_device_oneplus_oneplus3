/*
 * Copyright (C) 2015 NXP Semiconductors
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

extern "C"
{
    #include "nfa_ee_api.h"
}

    /*******************************************************************************
    **
    ** Function:        initialize
    **
    ** Description:     Initialize the channel.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    extern bool dwpChannelForceClose;
    INT16 open();
    bool close(INT16 mHandle);

    bool transceive (UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                     INT32 recvBufferMaxSize, INT32& recvBufferActualSize, INT32 timeoutMillisec);

    void doeSE_Reset();
    void doDwpChannel_ForceExit();
