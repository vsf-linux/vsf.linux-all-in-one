/*****************************************************************************
 *   Copyright(C)2009-2026 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

// Clean-room FreeRTOS stream_buffer.h shim for VSF.
//
// Contract (single-reader / single-writer):
//   - A stream buffer is a byte-oriented lock-free-ish ring buffer backed
//     by a pair of vsf_sem_t events (one for space-available, one for
//     data-available).
//   - Send blocks up to xTicksToWait until either (a) the full request
//     fits, or (b) the timeout expires, in which case it writes whatever
//     fits and returns the short count.
//   - Receive blocks until the ring holds at least xTriggerLevelBytes of
//     data, or the timeout expires. On return it copies up to
//     xBufferLengthBytes bytes out.
//   - FromISR variants never block; they do the I/O that can complete
//     immediately and report the byte count.

#ifndef __VSF_FREERTOS_STREAM_BUFFER_H__
#define __VSF_FREERTOS_STREAM_BUFFER_H__

#include "FreeRTOS.h"
#include <stddef.h>

#if defined(__VSF_FREERTOS_STREAM_BUFFER_CLASS_IMPLEMENT)
#   undef __VSF_FREERTOS_STREAM_BUFFER_CLASS_IMPLEMENT
#   define __VSF_CLASS_IMPLEMENT__
#endif

#include "utilities/ooc_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

// Forward-declared vsf_class. StreamBufferHandle_t is a StaticStreamBuffer_t *.
// Message buffers share the exact same layout; message_buffer.h typedefs
// StaticMessageBuffer_t as an alias to this type.
vsf_dcl_class(StaticStreamBuffer_t)
typedef StaticStreamBuffer_t *      StreamBufferHandle_t;

vsf_class(StaticStreamBuffer_t) {
    private_member(
        uint8_t *       buf;
        size_t          capacity;       // buffer size in bytes
        size_t          head;           // write index
        size_t          tail;           // read index
        size_t          count;          // bytes currently stored
        size_t          trigger_level;  // receive threshold
        bool            is_message;     // message buffer vs stream buffer
        bool            is_static;      // true when the control block is user-owned
        bool            is_buf_static;  // true when `buf` is user-owned
        vsf_sem_t       data_sem;       // posted on bytes-available edge
        vsf_sem_t       space_sem;      // posted on space-available edge
    )
};

/*============================ API ===========================================*/

extern StreamBufferHandle_t xStreamBufferCreate(size_t xBufferSizeBytes,
                                                size_t xTriggerLevelBytes);
extern void                 vStreamBufferDelete(StreamBufferHandle_t xStreamBuffer);

// Zero-heap create. The caller provides BOTH the ring storage
// (xBufferSizeBytes bytes) and the control-block buffer. Neither is
// freed by vStreamBufferDelete.
extern StreamBufferHandle_t xStreamBufferCreateStatic(
        size_t xBufferSizeBytes,
        size_t xTriggerLevelBytes,
        uint8_t *pucStreamBufferStorageArea,
        StaticStreamBuffer_t *pxStaticStreamBuffer);

// Writes up to xDataLengthBytes bytes. Blocks up to xTicksToWait while
// there is not enough space for the full payload; copies the prefix
// that fits on timeout. Returns the byte count actually written.
extern size_t xStreamBufferSend(StreamBufferHandle_t xStreamBuffer,
                                const void *pvTxData,
                                size_t xDataLengthBytes,
                                TickType_t xTicksToWait);

// Non-blocking ISR variant. pxHigherPriorityTaskWoken may be NULL.
// Returns bytes written (0..xDataLengthBytes).
extern size_t xStreamBufferSendFromISR(StreamBufferHandle_t xStreamBuffer,
                                       const void *pvTxData,
                                       size_t xDataLengthBytes,
                                       BaseType_t *pxHigherPriorityTaskWoken);

// Reads up to xBufferLengthBytes bytes. Blocks until the ring holds at
// least xTriggerLevelBytes of data (or the buffer is full) or the
// timeout expires. Returns the byte count copied to pvRxData.
extern size_t xStreamBufferReceive(StreamBufferHandle_t xStreamBuffer,
                                   void *pvRxData,
                                   size_t xBufferLengthBytes,
                                   TickType_t xTicksToWait);

extern size_t xStreamBufferReceiveFromISR(StreamBufferHandle_t xStreamBuffer,
                                          void *pvRxData,
                                          size_t xBufferLengthBytes,
                                          BaseType_t *pxHigherPriorityTaskWoken);

extern BaseType_t xStreamBufferIsEmpty(const StreamBufferHandle_t xStreamBuffer);
extern BaseType_t xStreamBufferIsFull(const StreamBufferHandle_t xStreamBuffer);
extern size_t     xStreamBufferBytesAvailable(const StreamBufferHandle_t xStreamBuffer);
extern size_t     xStreamBufferSpacesAvailable(const StreamBufferHandle_t xStreamBuffer);

extern BaseType_t xStreamBufferSetTriggerLevel(StreamBufferHandle_t xStreamBuffer,
                                               size_t xTriggerLevelBytes);
extern BaseType_t xStreamBufferReset(StreamBufferHandle_t xStreamBuffer);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_STREAM_BUFFER_H__
