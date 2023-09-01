/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

SampleSpec::SampleSpec()
    : sample_rate_(0) {
}

SampleSpec::SampleSpec(const size_t sample_rate, const ChannelSet& channel_set)
    : sample_rate_(sample_rate)
    , channel_set_(channel_set) {
    roc_panic_if_msg(sample_rate_ == 0, "sample spec: invalid sample rate");
    roc_panic_if_msg(channel_set_.layout() == ChanLayout_Invalid,
                     "sample spec: invalid channel layout");
    roc_panic_if_msg(channel_set_.num_channels() == 0,
                     "sample spec: invalid channel count");
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const ChannelLayout channel_layout,
                       const ChannelMask channel_mask)
    : sample_rate_(sample_rate)
    , channel_set_(channel_layout, channel_mask) {
    roc_panic_if_msg(sample_rate_ == 0, "sample spec: invalid sample rate");
}

bool SampleSpec::operator==(const SampleSpec& other) const {
    return sample_rate_ == other.sample_rate_ && channel_set_ == other.channel_set_;
}

bool SampleSpec::operator!=(const SampleSpec& other) const {
    return !(*this == other);
}

bool SampleSpec::is_valid() const {
    return sample_rate_ != 0 && channel_set_.is_valid();
}

size_t SampleSpec::sample_rate() const {
    return sample_rate_;
}

void SampleSpec::set_sample_rate(const size_t sample_rate) {
    sample_rate_ = sample_rate;
}

const ChannelSet& SampleSpec::channel_set() const {
    return channel_set_;
}

ChannelSet& SampleSpec::channel_set() {
    return channel_set_;
}

void SampleSpec::set_channel_set(const ChannelSet& channel_set) {
    channel_set_ = channel_set;
}

size_t SampleSpec::num_channels() const {
    return channel_set_.num_channels();
}

size_t SampleSpec::ns_2_samples_per_chan(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return (size_t)ns_2_rtp_timestamp(ns_duration);
}

core::nanoseconds_t SampleSpec::samples_per_chan_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return rtp_timestamp_2_ns((packet::timestamp_diff_t)n_samples);
}

size_t SampleSpec::ns_2_samples_overall(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return (size_t)ns_2_rtp_timestamp(ns_duration) * num_channels();
}

core::nanoseconds_t SampleSpec::samples_overall_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(n_samples % num_channels() != 0,
                     "sample spec: # of samples must be dividable by channels number");

    return rtp_timestamp_2_ns(packet::timestamp_diff_t(n_samples / num_channels()));
}

packet::timestamp_diff_t
SampleSpec::ns_2_rtp_timestamp(const core::nanoseconds_t ns_delta) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return packet::timestamp_diff_t(
        roundf(float(ns_delta) / core::Second * sample_rate_));
}

core::nanoseconds_t
SampleSpec::rtp_timestamp_2_ns(const packet::timestamp_diff_t rtp_delta) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return core::nanoseconds_t(roundf(float(rtp_delta) / sample_rate_ * core::Second));
}

} // namespace audio
} // namespace roc
