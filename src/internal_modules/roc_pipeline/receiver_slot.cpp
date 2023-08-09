/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_slot.h"
#include "roc_core/log.h"
#include "roc_pipeline/endpoint_helpers.h"

namespace roc {
namespace pipeline {

ReceiverSlot::ReceiverSlot(const ReceiverConfig& receiver_config,
                           ReceiverState& receiver_state,
                           audio::Mixer& mixer,
                           const rtp::FormatMap& format_map,
                           packet::PacketFactory& packet_factory,
                           core::BufferFactory<uint8_t>& byte_buffer_factory,
                           core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                           core::IArena& arena)
    : core::RefCounted<ReceiverSlot, core::ArenaAllocation>(arena)
    , format_map_(format_map)
    , receiver_state_(receiver_state)
    , session_group_(receiver_config,
                     receiver_state,
                     mixer,
                     format_map,
                     packet_factory,
                     byte_buffer_factory,
                     sample_buffer_factory,
                     arena) {
    roc_log(LogDebug, "receiver slot: initializing");
}

ReceiverEndpoint* ReceiverSlot::add_endpoint(address::Interface iface,
                                             address::Protocol proto) {
    roc_log(LogDebug, "receiver slot: adding %s endpoint %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    switch (iface) {
    case address::Iface_AudioSource:
        return create_source_endpoint_(proto);

    case address::Iface_AudioRepair:
        return create_repair_endpoint_(proto);

    case address::Iface_AudioControl:
        return create_control_endpoint_(proto);

    default:
        break;
    }

    roc_log(LogError, "receiver slot: unsupported interface");
    return NULL;
}

void ReceiverSlot::advance(packet::timestamp_t timestamp) {
    if (control_endpoint_) {
        control_endpoint_->pull_packets();
    }

    if (source_endpoint_) {
        source_endpoint_->pull_packets();
    }

    if (repair_endpoint_) {
        repair_endpoint_->pull_packets();
    }

    session_group_.advance_sessions(timestamp);
}

void ReceiverSlot::reclock(packet::ntp_timestamp_t timestamp) {
    session_group_.reclock_sessions(timestamp);
}

size_t ReceiverSlot::num_sessions() const {
    return session_group_.num_sessions();
}

ReceiverEndpoint* ReceiverSlot::create_source_endpoint_(address::Protocol proto) {
    if (source_endpoint_) {
        roc_log(LogError, "receiver slot: audio source endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioSource, proto)) {
        return NULL;
    }

    if (repair_endpoint_) {
        if (!validate_endpoint_pair_consistency(proto, repair_endpoint_->proto())) {
            return NULL;
        }
    }

    source_endpoint_.reset(new (source_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, format_map_, arena()));

    if (!source_endpoint_ || !source_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create source endpoint");
        source_endpoint_.reset(NULL);
        return NULL;
    }

    return source_endpoint_.get();
}

ReceiverEndpoint* ReceiverSlot::create_repair_endpoint_(address::Protocol proto) {
    if (repair_endpoint_) {
        roc_log(LogError, "receiver slot: audio repair endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioRepair, proto)) {
        return NULL;
    }

    if (source_endpoint_) {
        if (!validate_endpoint_pair_consistency(source_endpoint_->proto(), proto)) {
            return NULL;
        }
    }

    repair_endpoint_.reset(new (repair_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, format_map_, arena()));

    if (!repair_endpoint_ || !repair_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create repair endpoint");
        repair_endpoint_.reset(NULL);
        return NULL;
    }

    return repair_endpoint_.get();
}

ReceiverEndpoint* ReceiverSlot::create_control_endpoint_(address::Protocol proto) {
    if (control_endpoint_) {
        roc_log(LogError, "receiver slot: audio control endpoint is already set");
        return NULL;
    }

    if (!validate_endpoint(address::Iface_AudioControl, proto)) {
        return NULL;
    }

    control_endpoint_.reset(new (control_endpoint_) ReceiverEndpoint(
        proto, receiver_state_, session_group_, format_map_, arena()));

    if (!control_endpoint_ || !control_endpoint_->is_valid()) {
        roc_log(LogError, "receiver slot: can't create control endpoint");
        control_endpoint_.reset(NULL);
        return NULL;
    }

    return control_endpoint_.get();
}

} // namespace pipeline
} // namespace roc
