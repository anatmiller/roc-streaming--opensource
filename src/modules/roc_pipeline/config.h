/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/config.h
//! @brief Pipeline config.

#ifndef ROC_PIPELINE_CONFIG_H_
#define ROC_PIPELINE_CONFIG_H_

#include "roc_audio/latency_monitor.h"
#include "roc_audio/resampler.h"
#include "roc_audio/watchdog.h"
#include "roc_core/stddefs.h"
#include "roc_fec/config.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace pipeline {

//! Defaults.
enum {
    //! Number of samples per second.
    DefaultSampleRate = 44100,

    //! Channel mask.
    DefaultChannelMask = 0x3,

    //! Number of samples per packet per channel.
    DefaultPacketSize = 320,

    //! Minum latency relative to target latency.
    DefaultMinLatency = -1,

    //! Maximum latency relative to target latency.
    DefaultMaxLatency = 2
};

//! Protocol identifier.
enum Protocol {
    //! Protocol is not set.
    Proto_None,

    //! Bare RTP.
    Proto_RTP,

    //! RTP source packet + FECFRAME Reed-Solomon footer (m=8).
    Proto_RTP_RSm8_Source,

    //! FEC repair packet + FECFRAME Reed-Solomon header (m=8).
    Proto_RSm8_Repair,

    //! RTP source packet + FECFRAME LDPC footer.
    Proto_RTP_LDPC_Source,

    //! FEC repair packet + FECFRAME LDPC header.
    Proto_LDPC_Repair
};

//! Port parameters.
//! @remarks
//!  On receiver, defines a listened port parameters. On sender,
//!  defines a destination port parameters.
struct PortConfig {
    //! Port address.
    packet::Address address;

    //! Port protocol.
    Protocol protocol;

    PortConfig()
        : protocol(Proto_None) {
    }
};

//! Session parameters.
//! @remarks
//!  Defines per-session parameters on the receiver side.
struct SessionConfig {
    //! Channel mask.
    packet::channel_mask_t channels;

    //! Number of samples per packet per channel.
    size_t samples_per_packet;

    //! Target latency, number of samples.
    packet::timestamp_t latency;

    //! FEC scheme parameters.
    fec::Config fec;

    //! RTP validator parameters.
    rtp::ValidatorConfig rtp_validator;

    //! LatencyMonitor parameters.
    audio::LatencyMonitorConfig latency_monitor;

    //! Watchdog parameters.
    audio::WatchdogConfig watchdog;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Perform resampling to to compensate sender and receiver frequency difference.
    bool resampling;

    //! Insert weird beeps instead of silence on packet loss.
    bool beeping;

    SessionConfig()
        : channels(DefaultChannelMask)
        , samples_per_packet(DefaultPacketSize)
        , latency(DefaultPacketSize * 27)
        , watchdog(DefaultSampleRate)
        , resampling(false)
        , beeping(false) {
        latency_monitor.min_latency =
            (packet::signed_timestamp_t)latency * DefaultMinLatency;
        latency_monitor.max_latency =
            (packet::signed_timestamp_t)latency * DefaultMaxLatency;
    }
};

//! Receiver parameters.
struct ReceiverConfig {
    //! Default parameters for session.
    SessionConfig default_session;

    //! Number of samples per second per channel.
    size_t sample_rate;

    //! Channel mask.
    packet::channel_mask_t channels;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! Fill uninitialized data with large values to make them more noticeable.
    bool poisoning;

    ReceiverConfig()
        : sample_rate(DefaultSampleRate)
        , channels(DefaultChannelMask)
        , timing(false)
        , poisoning(false) {
    }
};

//! Sender parameters.
struct SenderConfig {
    //! Parameters for the port to which source packets are sent.
    PortConfig source_port;

    //! Parameters for the port to which repair packets are sent.
    PortConfig repair_port;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! FEC scheme parameters.
    fec::Config fec;

    //! Number of samples per second per channel.
    size_t sample_rate;

    //! Channel mask.
    packet::channel_mask_t channels;

    //! RTP payload type for audio packets.
    rtp::PayloadType payload_type;

    //! Number of samples per packet per channel.
    size_t samples_per_packet;

    //! Resample frames with a constant ratio.
    bool resampling;

    //! Interleave packets.
    bool interleaving;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! Fill unitialized data with large values to make them more noticable.
    bool poisoning;

    SenderConfig()
        : sample_rate(DefaultSampleRate)
        , channels(DefaultChannelMask)
        , payload_type(rtp::PayloadType_L16_Stereo)
        , samples_per_packet(DefaultPacketSize)
        , resampling(false)
        , interleaving(false)
        , timing(false)
        , poisoning(false) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
