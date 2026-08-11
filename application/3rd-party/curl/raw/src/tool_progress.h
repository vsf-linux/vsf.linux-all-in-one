#ifndef HEADER_CURL_TOOL_PROGRESS_H
#define HEADER_CURL_TOOL_PROGRESS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "tool_setup.h"

int xferinfo_cb(void *clientp,
                curl_off_t dltotal,
                curl_off_t dlnow,
                curl_off_t ultotal,
                curl_off_t ulnow);

bool progress_meter(struct GlobalConfig *global,
                    struct timeval *start,
                    bool final);
void progress_finalize(struct per_transfer *per);

#ifdef __VSF__
struct speedcount {
  curl_off_t dl;
  curl_off_t ul;
  struct timeval stamp;
};
struct __curl_tool_progress_ctx {
    curl_off_t all_dltotal;
    curl_off_t all_ultotal;
    curl_off_t all_dlalready;
    curl_off_t all_ulalready;
    curl_off_t all_xfers;

#define SPEEDCNT 10
    unsigned int speedindex;
    bool indexwrapped;
    struct speedcount speedstore[SPEEDCNT];

    struct {
        struct timeval stamp;
        bool header;
    } progress_meter;
};
declare_vsf_curl_mod(curl_tool_progress)
#   define curl_tool_progress_ctx   ((struct __curl_tool_progress_ctx *)vsf_linux_dynlib_ctx(&vsf_curl_mod_name(curl_tool_progress)))
#   define all_xfers                (curl_tool_progress_ctx->all_xfers)
#else
extern curl_off_t all_xfers;   /* total number */
#endif

#endif /* HEADER_CURL_TOOL_PROGRESS_H */
