/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/free_list_impl.h
//! @brief Intrusive doubly-linked list implementation.

#ifndef ROC_CORE_FREE_LIST_IMPL_H_
#define ROC_CORE_FREE_LIST_IMPL_H_

#include "roc_core/atomic.h"
#include "roc_core/free_list_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive singly-linked list implementation class.
//! Handles FreeList infrastructure independent of templated type for FreeList.
//! Ownership handling is left to the main FreeList class.
class FreeListImpl : public NonCopyable<> {
public:
    FreeListImpl();
    ~FreeListImpl();

    //! Get first list node.
    FreeListData* front() const;

    //! Remove first node and return.
    FreeListData* pop_front();

    //! Insert node into list.
    void push_front(FreeListData* node);

private:
    static void check_is_member_(const FreeListData* node, const FreeListImpl* list);

    //! Add node knowing that it is not part of a free list.
    void add_knowing_refcount_is_zero(FreeListData* node);

    Atomic<FreeListData*> head_;
    static const uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
    static const uint32_t REFS_MASK = 0x7FFFFFFF;
    static const uint32_t SUB_1 = 0xFFFFFFFF;
    static const uint32_t SUB_2 = 0xFFFFFFFE;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_FREE_LIST_IMPL_H_
