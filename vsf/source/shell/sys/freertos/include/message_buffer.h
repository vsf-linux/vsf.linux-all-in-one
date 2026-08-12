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

// Clean-room FreeRTOS message_buffer.h shim for VSF.
//
// Message buffers are a thin wrapper over stream buffers: each message
// is stored in the ring as a 4-byte little-endian length header followed
// by the payload bytes. The shim writes the header atomically with the
// payload; xMessageBufferSend either succeeds in full (header + payload)
// or blocks / fails as a whole.
//
// Receive never splits a message: if xBufferLengthBytes is smaller than
// the next pending message the call returns 0 and leaves the message in
// the buffer for a subsequent retry with a larger buffer.

#ifndef __VSF_FREERTOS_MESSAGE_BUFFER_H__
#define __VSF_FREERTOS_MESSAGE_BUFFER_H__

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ TYPES =========================================*/

// Message buffers and stream buffers share the same control-block layout;
// MessageBufferHandle_t is a pointer-compatible alias of StreamBufferHandle_t.
typedef StreamBufferHandle_t    MessageBufferHandle_t;
typedef StaticStreamBuffer_t    StaticMessageBuffer_t;

/*============================ API ===========================================*/

// xBufferSizeBytes must be large enough to hold at least one message
// plus its 4-byte header. Returns NULL on invalid args or OOM.
extern MessageBufferHandle_t xMessageBufferCreate(size_t xBufferSizeBytes);
extern void                  vMessageBufferDelete(MessageBufferHandle_t xMessageBuffer);

// Zero-heap create. Caller-owned storage; neither buffer is freed by
// vMessageBufferDelete.
extern MessageBufferHandle_t xMessageBufferCreateStatic(
        size_t xBufferSizeBytes,
        uint8_t *pucMessageBufferStorageArea,
        StaticMessageBuffer_t *pxStaticMessageBuffer);

// All-or-nothing semantics. Returns xDataLengthBytes on success or 0 on
// timeout / invalid args. Does NOT partially send.
extern size_t xMessageBufferSend(MessageBufferHandle_t xMessageBuffer,
                                 const void *pvTxData,
                                 size_t xDataLengthBytes,
                                 TickType_t xTicksToWait);

extern size_t xMessageBufferSendFromISR(MessageBufferHandle_t xMessageBuffer,
                                        const void *pvTxData,
                                        size_t xDataLengthBytes,
                                        BaseType_t *pxHigherPriorityTaskWoken);

// Returns the message length copied into pvRxData, or 0 on timeout or
// when xBufferLengthBytes is smaller than the head message.
extern size_t xMessageBufferReceive(MessageBufferHandle_t xMessageBuffer,
                                    void *pvRxData,
                                    size_t xBufferLengthBytes,
                                    TickType_t xTicksToWait);

extern size_t xMessageBufferReceiveFromISR(MessageBufferHandle_t xMessageBuffer,
                                           void *pvRxData,
                                           size_t xBufferLengthBytes,
                                           BaseType_t *pxHigherPriorityTaskWoken);

extern BaseType_t xMessageBufferIsEmpty(const MessageBufferHandle_t xMessageBuffer);
extern BaseType_t xMessageBufferIsFull(const MessageBufferHandle_t xMessageBuffer);
extern size_t     xMessageBufferSpaceAvailable(const MessageBufferHandle_t xMessageBuffer);
// Length (in bytes) of the next message to be read, or 0 if empty.
extern size_t     xMessageBufferNextLengthBytes(MessageBufferHandle_t xMessageBuffer);
extern BaseType_t xMessageBufferReset(MessageBufferHandle_t xMessageBuffer);

#ifdef __cplusplus
}
#endif

#endif      // __VSF_FREERTOS_MESSAGE_BUFFER_H__
