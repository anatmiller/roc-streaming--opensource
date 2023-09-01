/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/heap_arena.h
//! @brief Heap arena implementation.

#ifndef ROC_CORE_HEAP_ARENA_H_
#define ROC_CORE_HEAP_ARENA_H_

#include "roc_core/align_ops.h"
#include "roc_core/atomic.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Heap arena implementation.
//!
//! Uses malloc() and free().
//!
//! Supports memory "poisoning" to make memory-related bugs (out of bound writes, use
//! after free, etc) more noticeable.
//!
//! The memory is always maximum aligned. Thread-safe.
class HeapArena : public IArena, public NonCopyable<> {
public:
    HeapArena();
    ~HeapArena();

    //! Enable panic on leak in destructor, for all instances.
    static void enable_leak_detection();

    //! Get number of allocated blocks.
    size_t num_allocations() const;

    //! Allocate memory.
    virtual void* allocate(size_t size);

    //! Deallocate previously allocated memory.
    virtual void deallocate(void*);

private:
    struct Chunk {
        size_t size;
        AlignMax data[];
    };

    static int enable_leak_detection_;

    Atomic<int> num_allocations_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HEAP_ARENA_H_
