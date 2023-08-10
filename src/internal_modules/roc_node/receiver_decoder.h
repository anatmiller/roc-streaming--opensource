/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/receiver_decoder.h
//! @brief Receiver decoder node.

#ifndef ROC_NODE_RECEIVER_DECODER_H_
#define ROC_NODE_RECEIVER_DECODER_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/mutex.h"
#include "roc_node/context.h"
#include "roc_node/node.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/receiver_loop.h"

namespace roc {
namespace node {

//! Receiver decoder node.
class ReceiverDecoder : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    ReceiverDecoder(Context& context, const pipeline::ReceiverConfig& pipeline_config);

    //! Deinitialize.
    ~ReceiverDecoder();

    //! Check if successfully constructed.
    bool is_valid();

    //! Activate interface.
    bool activate(address::Interface iface, address::Protocol proto);

    //! Write packet for decoding.
    bool write(address::Interface iface, const packet::PacketPtr& packet);

    //! Source for reading decoded frames.
    sndio::ISource& source();

private:
    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex mutex_;

    core::Atomic<packet::IWriter*> endpoint_writers_[address::Iface_Max];

    pipeline::ReceiverLoop pipeline_;
    pipeline::ReceiverLoop::SlotHandle slot_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    bool valid_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_RECEIVER_DECODER_H_
