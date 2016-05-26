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
#ifdef __cplusplus

extern "C" {

#endif


#ifndef ALALIB_H_
#define ALALIB_H_

#include "IChannel.h"

/*******************************************************************************
**
** Function:        ALA_Init
**
** Description:     Initializes the ALA library and opens the DWP communication channel
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
unsigned char ALA_Init(IChannel *channel);

/*******************************************************************************
**
** Function:        ALA_Start
**
** Description:     Starts the ALA over DWP
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
unsigned char ALA_Start(const char *name, const char *dest, UINT8 *pdata, UINT16 len, UINT8 *respSW);
#else
unsigned char ALA_Start(const char *name, UINT8 *pdata, UINT16 len);
#endif

/*******************************************************************************
**
** Function:        ALA_DeInit
**
** Description:     Deinitializes the ALA Lib
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
bool ALA_DeInit();

#if(NXP_LDR_SVC_VER_2 == TRUE)
/*******************************************************************************
**
** Function:        ALA_lsGetVersion
**
** Description:     Get the Loader service Applet and cleint version
**
** Returns:         byte[] array.
**
*******************************************************************************/
unsigned char ALA_lsGetVersion(UINT8 *pVersion);
unsigned char ALA_lsGetStatus(UINT8 *pVersion);
unsigned char ALA_lsGetAppletStatus(UINT8 *pVersion);
#else
void ALA_GetlistofApplets(char *list[], UINT8* num);

unsigned char ALA_GetCertificateKey(UINT8 *pKey, INT32 *pKeylen);
#endif

inline int FSCANF_BYTE(FILE *stream, const char *format, void* pVal)
{
    int Result = 0;

    if((NULL != stream) && (NULL != format) && (NULL != pVal))
    {
        unsigned int dwVal;
        unsigned char* pTmp = (unsigned char*)pVal;
        Result = fscanf(stream, format, &dwVal);

        (*pTmp) = (unsigned char)(dwVal & 0x000000FF);
    }
    return Result;
}

#endif /* ALALIB_H_ */

#ifdef __cplusplus

}

#endif
