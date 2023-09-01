/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_spec.cpp"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

TEST_GROUP(samplespec) {};

TEST(samplespec, ns_2_int) {
    enum { SampleRate = 44100, NumCh = 2, ChMask = 0x3, FrameLen = 178 };
    const float fs = (float)SampleRate;

    for (size_t numch = 1; numch < 32; ++numch) {
        const audio::SampleSpec samplespec = SampleSpec(
            SampleRate, audio::ChanLayout_Surround, ChannelMask((1 << numch) - 1));

        CHECK_EQUAL(samplespec.channel_set().num_channels(), numch);

        CHECK_EQUAL(samplespec.num_channels(), numch);

        // Check rounding
        CHECK_EQUAL(samplespec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / fs * core::Second / 2 + 1)),
                    1);
        CHECK_EQUAL(samplespec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / fs * core::Second / 2 - 1)),
                    0);

        CHECK_EQUAL(
            samplespec.ns_2_samples_per_chan(core::nanoseconds_t(1 / fs * core::Second)),
            1);
        CHECK_EQUAL(
            samplespec.ns_2_samples_per_chan(core::nanoseconds_t(2 / fs * core::Second)),
            2);
        CHECK_EQUAL(
            samplespec.ns_2_rtp_timestamp(core::nanoseconds_t(1 / fs * core::Second)), 1);
        CHECK_EQUAL(
            samplespec.ns_2_rtp_timestamp(core::nanoseconds_t(2 / fs * core::Second)), 2);
        CHECK_EQUAL(
            samplespec.ns_2_samples_overall(core::nanoseconds_t(1 / fs * core::Second)),
            numch);
        CHECK_EQUAL(
            samplespec.ns_2_samples_overall(core::nanoseconds_t(2 / fs * core::Second)),
            numch * 2);
    }
}

TEST(samplespec, nsamples_2_ns) {
    enum { SampleRate = 44100, NumCh = 2, ChMask = 0x3, FrameLen = 178 };
    const double fs = (float)SampleRate;

    core::nanoseconds_t epsilon = core::nanoseconds_t(0.01 / fs * core::Second);

    for (size_t numch = 1; numch < 32; ++numch) {
        const audio::SampleSpec samplespec = SampleSpec(
            SampleRate, audio::ChanLayout_Surround, ChannelMask((1 << numch) - 1));

        CHECK_EQUAL(samplespec.channel_set().num_channels(), numch);

        CHECK_EQUAL(samplespec.num_channels(), numch);

        core::nanoseconds_t sampling_period = core::nanoseconds_t(1 / fs * core::Second);

        CHECK(core::ns_within_delta(samplespec.samples_per_chan_2_ns(1), sampling_period,
                                    epsilon));
        CHECK(core::ns_within_delta(samplespec.fract_samples_per_chan_2_ns(0.1f),
                                    core::nanoseconds_t(0.1 / fs * core::Second),
                                    epsilon));
        CHECK(core::ns_within_delta(samplespec.fract_samples_per_chan_2_ns(-0.1f),
                                    -core::nanoseconds_t(0.1 / fs * core::Second),
                                    epsilon));
        CHECK(core::ns_within_delta(samplespec.samples_overall_2_ns(numch),
                                    sampling_period, epsilon));
        CHECK(core::ns_within_delta(samplespec.rtp_timestamp_2_ns(1), sampling_period,
                                    epsilon));
    }
}

} // namespace audio
} // namespace roc
