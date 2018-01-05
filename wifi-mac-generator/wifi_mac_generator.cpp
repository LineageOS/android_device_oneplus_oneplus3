/*
 * Copyright 2016, The Android Open Source Project
 * Copyright 2018, The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG  "WifiMacGenerator"

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <sys/stat.h>

#include <android-base/logging.h>

/* wifi get mac */
static int force_random = 0;
static const char NV_MAC_FILE_0[]          = "/data/oemnvitems/4678_0";
static const char NV_MAC_FILE_1[]          = "/data/oemnvitems/4678_1";
static const char WLAN_MAC_BIN[]           = "/persist/wlan_mac.bin";
static const char STA_MAC_ADDR_NAME[]      = "Intf0MacAddress=";
static const char P2P_MAC_ADDR_NAME[]      = "Intf1MacAddress=";
static const char MAC_ADDR_NAME_NOT_USE1[] = "Intf3MacAddress=000AF58989FD\n";
static const char MAC_ADDR_NAME_NOT_USE2[] = "Intf4MacAddress=000AF58989FC\n";
static const char MAC_ADDR_NAME_END[]      = "END\n";
static const char MAC_ADDR_NAME_REN[]      = "\n";

static void to_upper(char *str) {
    int i = 0;
    while(str[i]!='\0') {
        if((str[i]>='a') && (str[i]<='z'))
            str[i]-=32;
        i++;
    }
}

static void array2str(uint8_t *array,char *str) {
    int i;
    char c;
    for (i = 0; i < 6; i++) {
        c = (array[i] >> 4) & 0x0f; //high 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'a'-32;
        }

        *str ++ = c;
        c = array[i] & 0x0f; //low 4 bit
        if(c >= 0 && c <= 9) {
            c += 0x30;
        }
        else if (c >= 0x0a && c <= 0x0f) {
            c = (c - 0x0a) + 'a'-32;
        }
        *str ++ = c;
    }
    *str = 0;
}

static int is_valid_mac_address(const char *pMacAddr) {
    int xdigit = 0;

    /* Mac full with zero */
    if (strcmp(pMacAddr, "000000000000") == 0) {
        return 0;
    }

    while (*pMacAddr) {
        if ((xdigit == 1) && ((*pMacAddr % 2) != 0))
            break;

        if (isxdigit(*pMacAddr)) {
            xdigit++;
        }
        ++pMacAddr;
    }
    return (xdigit == 12? 1 : 0);
}

static void update_wlan_mac_bin(uint8_t *mac, bool random) {
    FILE *fb = NULL;
    struct stat st;
    char buf [150];
    int i = 0;
    uint8_t staMac[6];
    uint8_t p2pMac[6];
    char wifi_addr[20];
    char p2p_addr[20];

    memset(buf, 0, 150);
    memset(wifi_addr, 0, 20);
    memset(p2p_addr, 0, 20);

    /* mac valid check */
    if (!random && mac != NULL) {
        for (i = 0; i < 6; i++) {
            staMac[i] = mac[6-i-1];
        }
        array2str(staMac, wifi_addr);
        if (!is_valid_mac_address(wifi_addr)) {//invalid mac
            ALOGE("%s: Invalid mac, will generate random mac", __func__);
            random = true;
        }
    }

    if (random) {
        /* If file is exist and check its size or force reproduce it when first start wifi */
        if (force_random == 0)
            force_random++;

        /* If file is exist and check its size */
        if (force_random != 1 && stat(WLAN_MAC_BIN, &st) == 0 && st.st_size >= 120) {
            ALOGD("%s: File %s already exists", __func__, WLAN_MAC_BIN);
            return;
        } else {
            srand(time(NULL));
            memset(wifi_addr, 0, 20);
            staMac[0] = 0xC0;
            staMac[1] = 0xEE;
            staMac[2] = 0xFB;
            staMac[3] = (rand() & 0x0FF00000) >> 20;
            staMac[4] = (rand() & 0x0FF00000) >> 20;
            staMac[5] = (rand() & 0x0FF00000) >> 20;
            memcpy(p2pMac, staMac, sizeof(staMac));
            array2str(staMac, wifi_addr);

            if (force_random == 1)
                force_random ++;
        }
    } else {
        memcpy(p2pMac, staMac, sizeof(staMac));
    }
    /* p2p device address */
    p2pMac[0] = p2pMac[0] | 0x02;
    array2str(p2pMac, p2p_addr);

    snprintf(buf, sizeof(buf), "%s%s%s%s%s%s%s%s%s",
             STA_MAC_ADDR_NAME, wifi_addr, MAC_ADDR_NAME_REN,
             P2P_MAC_ADDR_NAME, p2p_addr, MAC_ADDR_NAME_REN,
             MAC_ADDR_NAME_NOT_USE1,
             MAC_ADDR_NAME_NOT_USE2,
             MAC_ADDR_NAME_END);

    ALOGV("%s: Buffer: %s", __func__, buf);

    fb = fopen(WLAN_MAC_BIN, "wb");
    if (fb != NULL) {
        ALOGD("%s: Writing buffer to file %s", __func__, WLAN_MAC_BIN);
        fwrite(buf, strlen(buf), 1, fb);
        fclose(fb);
    }
}

void get_mac_from_nv() {
    struct stat st;
    FILE * fd;
    uint8_t buf[6] = {0};
    int len = 0;
    int retries = 10;
    bool random = false;

    // Wait till NV_MAC_FILE is available after factory reset
    while (retries-- > 0) {
        if ((stat(NV_MAC_FILE_0, &st) == 0) && (stat(NV_MAC_FILE_1, &st) == 0)) break;
        ALOGD("%s: NV mac files %s | %s do not exist yet. Waiting... (%d)", __func__, NV_MAC_FILE_0, NV_MAC_FILE_1, retries);
        usleep(1000000);
    }

    if ((stat(NV_MAC_FILE_0, &st) != 0 || st.st_size < 6) &&
        (stat(NV_MAC_FILE_1, &st) != 0 || st.st_size < 6)) {
        ALOGE("%s: Invalid NV mac files %s | %s, will generate random mac", __func__, NV_MAC_FILE_0, NV_MAC_FILE_1);
        random = true;
    } else {
        // read nv files in binary mode
        if ((fd = fopen(NV_MAC_FILE_0, "rb")) == NULL) {
            ALOGE("%s: Could not open NV mac file %s", __func__, NV_MAC_FILE_0);
            if ((fd = fopen(NV_MAC_FILE_1, "rb")) == NULL) {
                ALOGE("%s: Could not open NV mac file %s", __func__, NV_MAC_FILE_1);
                random = true;
            }
        }
    }
    if (!random) {
        fseek(fd, 0, SEEK_SET);
        len = fread(buf, sizeof(char), st.st_size, fd);
        fclose(fd);
    }
    update_wlan_mac_bin(buf, random);
}

int main()
{
    get_mac_from_nv();
    return 0;
}
