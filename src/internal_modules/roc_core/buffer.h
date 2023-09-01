/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer.h
//! @brief Buffer.

#ifndef ROC_CORE_BUFFER_H_
#define ROC_CORE_BUFFER_H_

#include "roc_core/pool.h"
#include "roc_core/ref_counted.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Fixed-size dynamically-allocated buffer.
//!
//! @tparam T defines element type.
//!
//! @remarks
//!  Buffer size is fixed, but determined at runtime, not compile time.
//!  It is defined by the pool that allocates the buffer.
//!  User typically works with buffers via Slice, which holds a reference
//!  to buffer and points to a variable-size subset of its memory.
//!
//! @see BufferFactory, Slice.
template <class T> class Buffer : public RefCounted<Buffer<T>, PoolAllocation> {
public:
    //! Initialize empty buffer.
    explicit Buffer(IPool& buffer_pool, size_t buffer_size)
        : RefCounted<Buffer<T>, PoolAllocation>(buffer_pool)
        , buffer_size_(buffer_size) {
        new (data()) T[size()];
    }

    //! Get number of elements in buffer.
    size_t size() const {
        return buffer_size_;
    }

    //! Get buffer data.
    T* data() {
        return (T*)(((char*)this) + sizeof(Buffer));
    }

    //! Get pointer to buffer from the pointer to its data.
    static Buffer* container_of(void* data) {
        return (Buffer*)((char*)data - sizeof(Buffer));
    }

private:
    const size_t buffer_size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_H_
