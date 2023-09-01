/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/concurrent_queue.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

ConcurrentQueue::ConcurrentQueue(Mode mode) {
    if (mode == Blocking) {
        write_sem_.reset(new (write_sem_) core::Semaphore());
    }
}

PacketPtr ConcurrentQueue::read() {
    core::Mutex::Lock lock(read_mutex_);

    if (write_sem_) {
        write_sem_->wait();
    }

    return queue_.pop_front_exclusive();
}

void ConcurrentQueue::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("concurrent queue: packet is null");
    }

    queue_.push_back(*packet);

    if (write_sem_) {
        write_sem_->post();
    }
}

} // namespace packet
} // namespace roc
