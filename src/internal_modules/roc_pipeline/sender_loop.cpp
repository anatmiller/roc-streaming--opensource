/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_loop.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/thread.h"
#include "roc_pipeline/sender_endpoint.h"

namespace roc {
namespace pipeline {

SenderLoop::Task::Task()
    : func_(NULL)
    , slot_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , outbound_writer_(NULL)
    , inbound_writer_(NULL)
    , slot_metrics_(NULL)
    , party_metrics_(NULL)
    , party_count_(NULL) {
}

SenderLoop::Tasks::CreateSlot::CreateSlot(const SenderSlotConfig& slot_config) {
    func_ = &SenderLoop::task_create_slot_;
    slot_config_ = slot_config;
}

SenderLoop::SlotHandle SenderLoop::Tasks::CreateSlot::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(slot_);
    return (SlotHandle)slot_;
}

SenderLoop::Tasks::DeleteSlot::DeleteSlot(SlotHandle slot) {
    func_ = &SenderLoop::task_delete_slot_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
}

SenderLoop::Tasks::QuerySlot::QuerySlot(SlotHandle slot,
                                        SenderSlotMetrics& slot_metrics,
                                        SenderParticipantMetrics* party_metrics,
                                        size_t* party_count) {
    func_ = &SenderLoop::task_query_slot_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    slot_metrics_ = &slot_metrics;
    party_metrics_ = party_metrics;
    party_count_ = party_count;
}

SenderLoop::Tasks::AddEndpoint::AddEndpoint(SlotHandle slot,
                                            address::Interface iface,
                                            address::Protocol proto,
                                            const address::SocketAddr& outbound_address,
                                            packet::IWriter& outbound_writer) {
    func_ = &SenderLoop::task_add_endpoint_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    iface_ = iface;
    proto_ = proto;
    outbound_address_ = outbound_address;
    outbound_writer_ = &outbound_writer;
}

packet::IWriter* SenderLoop::Tasks::AddEndpoint::get_inbound_writer() const {
    if (!success()) {
        return NULL;
    }
    return inbound_writer_;
}

SenderLoop::SenderLoop(IPipelineTaskScheduler& scheduler,
                       const SenderSinkConfig& sink_config,
                       const rtp::EncodingMap& encoding_map,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IArena& arena)
    : PipelineLoop(scheduler, sink_config.pipeline_loop, sink_config.input_sample_spec)
    , sink_(sink_config,
            encoding_map,
            packet_factory,
            byte_buffer_factory,
            sample_buffer_factory,
            arena)
    , ticker_ts_(0)
    , auto_duration_(sink_config.enable_auto_duration)
    , auto_cts_(sink_config.enable_auto_cts)
    , sample_spec_(sink_config.input_sample_spec)
    , valid_(false) {
    if (!sink_.is_valid()) {
        return;
    }

    if (sink_config.enable_timing) {
        ticker_.reset(new (ticker_)
                          core::Ticker(sink_config.input_sample_spec.sample_rate()));
        if (!ticker_) {
            return;
        }
    }

    valid_ = true;
}

bool SenderLoop::is_valid() const {
    return valid_;
}

sndio::ISink& SenderLoop::sink() {
    roc_panic_if_not(is_valid());

    return *this;
}

sndio::ISink* SenderLoop::to_sink() {
    roc_panic_if(!is_valid());

    return this;
}

sndio::ISource* SenderLoop::to_source() {
    roc_panic_if(!is_valid());

    return NULL;
}

sndio::DeviceType SenderLoop::type() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.type();
}

sndio::DeviceState SenderLoop::state() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.state();
}

void SenderLoop::pause() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    sink_.pause();
}

bool SenderLoop::resume() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.resume();
}

bool SenderLoop::restart() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.restart();
}

audio::SampleSpec SenderLoop::sample_spec() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.sample_spec();
}

core::nanoseconds_t SenderLoop::latency() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.latency();
}

bool SenderLoop::has_latency() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_latency();
}

bool SenderLoop::has_clock() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_clock();
}

void SenderLoop::write(audio::Frame& frame) {
    roc_panic_if_not(is_valid());

    if (auto_duration_) {
        if (frame.has_duration()) {
            roc_panic("sender loop: unexpected non-zero duration in auto-duration mode");
        }
        frame.set_duration(sample_spec_.bytes_2_stream_timestamp(frame.num_bytes()));
    }

    if (auto_cts_) {
        if (frame.capture_timestamp() != 0) {
            roc_panic("sender loop: unexpected non-zero cts in auto-cts mode");
        }
        frame.set_capture_timestamp(core::timestamp(core::ClockUnix));
    }

    core::Mutex::Lock lock(sink_mutex_);

    if (ticker_) {
        ticker_->wait(ticker_ts_);
        ticker_ts_ += frame.duration();
    }

    // invokes process_subframe_imp() and process_task_imp()
    if (!process_subframes_and_tasks(frame)) {
        return;
    }
}

core::nanoseconds_t SenderLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

uint64_t SenderLoop::tid_imp() const {
    return core::Thread::get_tid();
}

bool SenderLoop::process_subframe_imp(audio::Frame& frame) {
    sink_.write(frame);

    // TODO: handle returned deadline and schedule refresh
    sink_.refresh(core::timestamp(core::ClockUnix));

    return true;
}

bool SenderLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool SenderLoop::task_create_slot_(Task& task) {
    task.slot_ = sink_.create_slot(task.slot_config_);
    return (bool)task.slot_;
}

bool SenderLoop::task_delete_slot_(Task& task) {
    roc_panic_if(!task.slot_);

    sink_.delete_slot(task.slot_);
    return true;
}

bool SenderLoop::task_query_slot_(Task& task) {
    roc_panic_if(!task.slot_);
    roc_panic_if(!task.slot_metrics_);

    task.slot_->get_metrics(*task.slot_metrics_, task.party_metrics_, task.party_count_);
    return true;
}

bool SenderLoop::task_add_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    SenderEndpoint* endpoint = task.slot_->add_endpoint(
        task.iface_, task.proto_, task.outbound_address_, *task.outbound_writer_);
    if (!endpoint) {
        return false;
    }
    task.inbound_writer_ = endpoint->inbound_writer();
    return true;
}

} // namespace pipeline
} // namespace roc
