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

#define LOG_TAG "NxpNfcNciHal"

#include <utils/Log.h>
#include <errno.h>
#include <hardware/hardware.h>
#include <hardware/nfc.h>
#include <phNxpNciHal_Adaptation.h>
#include <string.h>
#include <stdlib.h>
/*****************************************************************************
 * NXP NCI HAL Function implementations.
 *****************************************************************************/

/*******************************************************************************
**
** Function         hal_open
**
** Description      It opens and initialzes the physical connection with NFCC.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_open(const struct nfc_nci_device *p_dev,
        nfc_stack_callback_t p_hal_cback,
        nfc_stack_data_callback_t *p_hal_data_callback)
{
    int retval = 0;

    pn547_dev_t *dev = (pn547_dev_t*) p_dev;
    retval = phNxpNciHal_open(p_hal_cback, p_hal_data_callback);

    return retval;
}

/*******************************************************************************
**
** Function         hal_write
**
** Description      Write the data to NFCC.
**
** Returns          Number of bytes successfully written to NFCC.
**
*******************************************************************************/
static int hal_write(const struct nfc_nci_device *p_dev, uint16_t data_len,
        const uint8_t *p_data)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_write(data_len, p_data);
    return retval;
}

/*******************************************************************************
**
** Function         hal_ioctl
**
** Description      Invoke ioctl to  to NFCC driver.
**
** Returns          status code of ioctl.
**
*******************************************************************************/
static int hal_ioctl(const struct nfc_nci_device *p_dev, long arg, void *p_data)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_ioctl(arg, p_data);
    return retval;
}

/*******************************************************************************
**
** Function         hal_core_initialized
**
** Description      Notify NFCC after successful initialization of NFCC.
**                  All proprietary settings can be done here.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_core_initialized(const struct nfc_nci_device *p_dev,
        uint8_t* p_core_init_rsp_params)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_core_initialized(p_core_init_rsp_params);
    return retval;
}

/*******************************************************************************
**
** Function         hal_pre_discover
**
** Description      Notify NFCC before start discovery.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_pre_discover(const struct nfc_nci_device *p_dev)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_pre_discover();
    return retval;
}

/*******************************************************************************
**
** Function         hal_close
**
** Description      Close the NFCC interface and free all resources.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_close(const struct nfc_nci_device *p_dev)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_close();
    return retval;
}

/*******************************************************************************
**
** Function         hal_control_granted
**
** Description      Notify NFCC that control is granted to HAL.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_control_granted(const struct nfc_nci_device *p_dev)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_control_granted();
    return retval;
}

/*******************************************************************************
**
** Function         hal_power_cycle
**
** Description      Notify power cycling has performed.
**
** Returns          0 if successful
**
*******************************************************************************/
static int hal_power_cycle(const struct nfc_nci_device *p_dev)
{
    int retval = 0;
    pn547_dev_t* dev = (pn547_dev_t*) p_dev;

    retval = phNxpNciHal_power_cycle();
    return retval;
}

/*************************************
 * Generic device handling.
 *************************************/

/*******************************************************************************
**
** Function         nfc_close
**
** Description      Close the nfc device instance.
**
** Returns          0 if successful
**
*******************************************************************************/
static int nfc_close(hw_device_t *dev)
{
    int retval = 0;
    free(dev);
    return retval;
}

/*******************************************************************************
**
** Function         nfc_open
**
** Description      Open the nfc device instance.
**
** Returns          0 if successful
**
*******************************************************************************/
static int nfc_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    ALOGD("%s: enter; name=%s", __FUNCTION__, name);
    int retval = 0; /* 0 is ok; -1 is error */
    pn547_dev_t *dev = NULL;
    if (strcmp(name, NFC_NCI_CONTROLLER) == 0)
    {
        dev = calloc(1, sizeof(pn547_dev_t));
        if(dev == NULL)
        {
            retval = -EINVAL;
        }
        else
        {
            /* Common hw_device_t fields */
            dev->nci_device.common.tag = HARDWARE_DEVICE_TAG;
            dev->nci_device.common.version = 0x00010000; /* [31:16] major, [15:0] minor */
            dev->nci_device.common.module = (struct hw_module_t*) module;
            dev->nci_device.common.close = nfc_close;

            /* NCI HAL method pointers */
            dev->nci_device.open = hal_open;
            dev->nci_device.write = hal_write;
            dev->nci_device.ioctl = hal_ioctl;
            dev->nci_device.core_initialized = hal_core_initialized;
            dev->nci_device.pre_discover = hal_pre_discover;
            dev->nci_device.close = hal_close;
            dev->nci_device.control_granted = hal_control_granted;
            dev->nci_device.power_cycle = hal_power_cycle;
            *device = (hw_device_t*) dev;
        }
    }
    else
    {
        retval = -EINVAL;
    }

    ALOGD("%s: exit %d", __FUNCTION__, retval);
    return retval;
}

/* Android hardware module definition */
static struct hw_module_methods_t nfc_module_methods =
{
    .open = nfc_open,
};

/* NFC module definition */
struct nfc_nci_module_t HAL_MODULE_INFO_SYM =
{
    .common =
    {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = 0x0100, /* [15:8] major, [7:0] minor (1.0) */
        .hal_api_version = 0x00, /* 0 is only valid value */
        .id = NFC_NCI_NXP_PN54X_HARDWARE_MODULE_ID,
        .name = "NXP PN54X NFC NCI HW HAL",
        .author = "NXP Semiconductors",
        .methods = &nfc_module_methods,
    },
};
