/*
 * Copyright (C) 2015, The CyanogenMod Project
 * Copyright (C) 2017-2019, The LineageOS Project
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "edify/expr.h"
#include "otautil/error_code.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ALPHABET_LEN 256

#ifdef USES_BOOTDEVICE_PATH
#define MODEM_PART_PATH "/dev/block/bootdevice/by-name/modem"
#else
#define MODEM_PART_PATH "/dev/block/platform/msm_sdcc.1/by-name/modem"
#endif
#define MODEM_VER_STR "Time_Stamp\": \""
#define MODEM_VER_STR_LEN 14
#define MODEM_VER_BUF_LEN 20

/* Boyer-Moore string search implementation from Wikipedia */

/* Return longest suffix length of suffix ending at str[p] */
static int max_suffix_len(const char *str, size_t str_len, size_t p) {
    uint32_t i;

    for (i = 0; (str[p - i] == str[str_len - 1 - i]) && (i < p); ) {
        i++;
    }

    return i;
}

/* Generate table of distance between last character of pat and rightmost
 * occurrence of character c in pat
 */
static void bm_make_delta1(int *delta1, const char *pat, size_t pat_len) {
    uint32_t i;
    for (i = 0; i < ALPHABET_LEN; i++) {
        delta1[i] = pat_len;
    }
    for (i = 0; i < pat_len - 1; i++) {
        uint8_t idx = (uint8_t) pat[i];
        delta1[idx] = pat_len - 1 - i;
    }
}

/* Generate table of next possible full match from mismatch at pat[p] */
static void bm_make_delta2(int *delta2, const char *pat, size_t pat_len) {
    int p;
    uint32_t last_prefix = pat_len - 1;

    for (p = pat_len - 1; p >= 0; p--) {
        /* Compare whether pat[p-pat_len] is suffix of pat */
        if (strncmp(pat + p, pat, pat_len - p) == 0) {
            last_prefix = p + 1;
        }
        delta2[p] = last_prefix + (pat_len - 1 - p);
    }

    for (p = 0; p < (int) pat_len - 1; p++) {
        /* Get longest suffix of pattern ending on character pat[p] */
        int suf_len = max_suffix_len(pat, pat_len, p);
        if (pat[p - suf_len] != pat[pat_len - 1 - suf_len]) {
            delta2[pat_len - 1 - suf_len] = pat_len - 1 - p + suf_len;
        }
    }
}

static char * bm_search(const char *str, size_t str_len, const char *pat,
        size_t pat_len) {
    int delta1[ALPHABET_LEN];
    int delta2[pat_len];
    int i;

    bm_make_delta1(delta1, pat, pat_len);
    bm_make_delta2(delta2, pat, pat_len);

    if (pat_len == 0) {
        return (char *) str;
    }

    i = pat_len - 1;
    while (i < (int) str_len) {
        int j = pat_len - 1;
        while (j >= 0 && (str[i] == pat[j])) {
            i--;
            j--;
        }
        if (j < 0) {
            return (char *) (str + i + 1);
        }
        i += MAX(delta1[(uint8_t) str[i]], delta2[j]);
    }

    return NULL;
}

static int get_modem_version(char *ver_str, size_t len) {
    int ret = 0;
    int fd;
    int modem_size;
    char *modem_data = NULL;
    char *offset = NULL;

    fd = open(MODEM_PART_PATH, O_RDONLY);
    if (fd < 0) {
        ret = errno;
        goto err_ret;
    }

    modem_size = lseek64(fd, 0, SEEK_END);
    if (modem_size == -1) {
        ret = errno;
        goto err_fd_close;
    }

    modem_data = (char *) mmap(NULL, modem_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (modem_data == (char *)-1) {
        ret = errno;
        goto err_fd_close;
    }

    /* Do Boyer-Moore search across MODEM data */
    offset = bm_search(modem_data, modem_size, MODEM_VER_STR, MODEM_VER_STR_LEN);
    if (offset != NULL) {
        snprintf(ver_str, len, "%s", offset + MODEM_VER_STR_LEN);
    } else {
        ret = -ENOENT;
    }

    munmap(modem_data, modem_size);
err_fd_close:
    close(fd);
err_ret:
    return ret;
}

/* verify_modem("MODEM_VERSION") */
Value* VerifyModemFn(const char *name, State *state, const std::vector<std::unique_ptr<Expr>>& argv) {
    char current_modem_version[MODEM_VER_BUF_LEN];
    int ret;
    struct tm tm1, tm2;

    ret = get_modem_version(current_modem_version, MODEM_VER_BUF_LEN);
    if (ret) {
        return ErrorAbort(state, kVendorFailure,
                "%s() failed to read current MODEM build time-stamp: %d", name, ret);
    }

    std::string modem_version;
    if (argv.empty() || !Evaluate(state, argv[0], &modem_version)) {
        return ErrorAbort(state, kArgsParsingFailure, "%s() error parsing arguments", name);
    }

    memset(&tm1, 0, sizeof(tm));
    strptime(current_modem_version, "%Y-%m-%d %H:%M:%S", &tm1);

    memset(&tm2, 0, sizeof(tm));
    strptime(modem_version.c_str(), "%Y-%m-%d %H:%M:%S", &tm2);

    if (mktime(&tm1) >= mktime(&tm2)) {
        ret = 1;
    }

    return StringValue(std::to_string(ret));
}

void Register_librecovery_updater_op3() {
    RegisterFunction("op3.verify_modem", VerifyModemFn);
}
