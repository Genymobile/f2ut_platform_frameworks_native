/*
 * Copyright 2014 The Android Open Source Project
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

#include <gui/BufferItem.h>

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <system/window.h>

namespace android {

BufferItem::BufferItem() :
    mTransform(0),
    mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
    mTimestamp(0),
    mIsAutoTimestamp(false),
    mFrameNumber(0),
    mSlot(INVALID_BUFFER_SLOT),
    mIsDroppable(false),
    mAcquireCalled(false),
    mTransformToDisplayInverse(false) {
    mCrop.makeInvalid();
    mDirtyRect.makeInvalid();
}

BufferItem::~BufferItem() {}

BufferItem::operator IGraphicBufferConsumer::BufferItem() const {
    IGraphicBufferConsumer::BufferItem bufferItem;
    bufferItem.mGraphicBuffer = mGraphicBuffer;
    bufferItem.mFence = mFence;
    bufferItem.mCrop = mCrop;
    bufferItem.mDirtyRect = mDirtyRect;
    bufferItem.mTransform = mTransform;
    bufferItem.mScalingMode = mScalingMode;
    bufferItem.mTimestamp = mTimestamp;
    bufferItem.mIsAutoTimestamp = mIsAutoTimestamp;
    bufferItem.mFrameNumber = mFrameNumber;
    bufferItem.mBuf = mSlot;
    bufferItem.mIsDroppable = mIsDroppable;
    bufferItem.mAcquireCalled = mAcquireCalled;
    bufferItem.mTransformToDisplayInverse = mTransformToDisplayInverse;
    return bufferItem;
}

template <typename T>
static void addAligned(size_t& size, T /* value */) {
    size = FlattenableUtils::align<sizeof(T)>(size);
    size += sizeof(T);
}

size_t BufferItem::getPodSize() const {
    // Must align<8> before writing these fields for this to be correct
    size_t size = 0;
    addAligned(size, mCrop);
#ifdef QCOM_BSP
    addAligned(size, mDirtyRect);
#endif
    addAligned(size, mTransform);
    addAligned(size, mScalingMode);
    addAligned(size, mTimestamp);
    addAligned(size, mIsAutoTimestamp);
    addAligned(size, mFrameNumber);
    addAligned(size, mSlot);
    addAligned(size, mIsDroppable);
    addAligned(size, mAcquireCalled);
    addAligned(size, mTransformToDisplayInverse);
    return size;
}

size_t BufferItem::getFlattenedSize() const {
    size_t size = sizeof(int32_t); // Flags
    if (mGraphicBuffer != 0) {
        size += mGraphicBuffer->getFlattenedSize();
        FlattenableUtils::align<4>(size);
    }
    if (mFence != 0) {
        size += mFence->getFlattenedSize();
        FlattenableUtils::align<4>(size);
    }
    size = FlattenableUtils::align<8>(size);
    return size + getPodSize();
}

size_t BufferItem::getFdCount() const {
    size_t count = 0;
    if (mGraphicBuffer != 0) {
        count += mGraphicBuffer->getFdCount();
    }
    if (mFence != 0) {
        count += mFence->getFdCount();
    }
    return count;
}

template <typename T>
static void writeAligned(void*& buffer, size_t& size, T value) {
    size -= FlattenableUtils::align<alignof(T)>(buffer);
    FlattenableUtils::write(buffer, size, value);
}

status_t BufferItem::flatten(
        void*& buffer, size_t& size, int*& fds, size_t& count) const {

    // make sure we have enough space
    if (size < BufferItem::getFlattenedSize()) {
        return NO_MEMORY;
    }

    // content flags are stored first
    uint32_t& flags = *static_cast<uint32_t*>(buffer);

    // advance the pointer
    FlattenableUtils::advance(buffer, size, sizeof(uint32_t));

    flags = 0;
    if (mGraphicBuffer != 0) {
        status_t err = mGraphicBuffer->flatten(buffer, size, fds, count);
        if (err) return err;
        size -= FlattenableUtils::align<4>(buffer);
        flags |= 1;
    }
    if (mFence != 0) {
        status_t err = mFence->flatten(buffer, size, fds, count);
        if (err) return err;
        size -= FlattenableUtils::align<4>(buffer);
        flags |= 2;
    }

    // Must align<8> so that getPodSize returns the correct value
    size -= FlattenableUtils::align<8>(buffer);

    // Check we still have enough space
    if (size < getPodSize()) {
        return NO_MEMORY;
    }

    writeAligned(buffer, size, mCrop);
#ifdef QCOM_BSP
    writeAligned(buffer, size, mDirtyRect);
#endif
    writeAligned(buffer, size, mTransform);
    writeAligned(buffer, size, mScalingMode);
    writeAligned(buffer, size, mTimestamp);
    writeAligned(buffer, size, mIsAutoTimestamp);
    writeAligned(buffer, size, mFrameNumber);
    writeAligned(buffer, size, mSlot);
    writeAligned(buffer, size, mIsDroppable);
    writeAligned(buffer, size, mAcquireCalled);
    writeAligned(buffer, size, mTransformToDisplayInverse);

    return NO_ERROR;
}

template <typename T>
static void readAligned(const void*& buffer, size_t& size, T& value) {
    size -= FlattenableUtils::align<alignof(T)>(buffer);
    FlattenableUtils::read(buffer, size, value);
}

status_t BufferItem::unflatten(
        void const*& buffer, size_t& size, int const*& fds, size_t& count) {

    if (size < sizeof(uint32_t)) {
        return NO_MEMORY;
    }

    uint32_t flags = 0;
    FlattenableUtils::read(buffer, size, flags);

    if (flags & 1) {
        mGraphicBuffer = new GraphicBuffer();
        status_t err = mGraphicBuffer->unflatten(buffer, size, fds, count);
        if (err) return err;
        size -= FlattenableUtils::align<4>(buffer);
    }

    if (flags & 2) {
        mFence = new Fence();
        status_t err = mFence->unflatten(buffer, size, fds, count);
        if (err) return err;
        size -= FlattenableUtils::align<4>(buffer);
    }

    // Must align<8> so that getPodSize returns the correct value
    size -= FlattenableUtils::align<8>(buffer);

    // Check we still have enough space
    if (size < getPodSize()) {
        return NO_MEMORY;
    }

    readAligned(buffer, size, mCrop);
#ifdef QCOM_BSP
    readAligned(buffer, size, mDirtyRect);
#endif
    readAligned(buffer, size, mTransform);
    readAligned(buffer, size, mScalingMode);
    readAligned(buffer, size, mTimestamp);
    readAligned(buffer, size, mIsAutoTimestamp);
    readAligned(buffer, size, mFrameNumber);
    readAligned(buffer, size, mSlot);
    readAligned(buffer, size, mIsDroppable);
    readAligned(buffer, size, mAcquireCalled);
    readAligned(buffer, size, mTransformToDisplayInverse);

    return NO_ERROR;
}

const char* BufferItem::scalingModeName(uint32_t scalingMode) {
    switch (scalingMode) {
        case NATIVE_WINDOW_SCALING_MODE_FREEZE: return "FREEZE";
        case NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW: return "SCALE_TO_WINDOW";
        case NATIVE_WINDOW_SCALING_MODE_SCALE_CROP: return "SCALE_CROP";
        default: return "Unknown";
    }
}

} // namespace android
