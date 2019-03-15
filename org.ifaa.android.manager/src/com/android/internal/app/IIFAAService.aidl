/********************************************************************************
** Copyright (C), 2014-2017, OnePlus Mobile Comm Corp., Ltd
** VENDOR_EDIT, All rights reserved.
**
** File: - IIFAAService.aidl
** Description:
**     IFAAService service interface
**
** ------------------------------- Revision History: ----------------------------
** <author>            <date>       <version>      <desc>
** ------------------------------------------------------------------------------
** KenShen@Framework     2017-03-30   v1       add init version.

********************************************************************************/

package com.android.internal.app;

interface IIFAAService {
   byte[] processCmdV2(in byte[] dataIn);
}
