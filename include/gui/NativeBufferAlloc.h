/*
 * Copyright (C) 2013 Canonical Ltd
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
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#ifndef NATIVE_BUFFER_ALLOC_H
#define NATIVE_BUFFER_ALLOC_H

namespace android
{
class NativeBufferAlloc : public RefBase
{
public:
    NativeBufferAlloc();
    virtual ~NativeBufferAlloc();
    virtual sp<GraphicBuffer> createGraphicBuffer(uint32_t w, uint32_t h,
        PixelFormat format, uint32_t usage, status_t* error);
};
}

#endif /* NATIVE_BUFFER_ALLOC_H */
