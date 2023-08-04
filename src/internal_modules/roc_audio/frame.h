/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/frame.h
//! @brief Audio frame.

#ifndef ROC_AUDIO_FRAME_H_
#define ROC_AUDIO_FRAME_H_

#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Audio frame.
class Frame : public core::NonCopyable<> {
public:
    //! Construct frame from samples.
    //! @remarks
    //!  The pointer is saved in the frame, no copying is performed.
Frame(sample_t *samples, size_t num_samples, const SampleSpec &spec, packet::ntp_timestamp_t ts = 0);

    //! Sub-frame copy constructor.
    Frame(Frame &frame, const core::nanoseconds_t offset, const core::nanoseconds_t max_duration = 0);

    //! Frame flags.
    enum {
        //! Set if the frame has at least some samples from packets.
        //! If this flag is clear, frame is completely zero because of lack of packets.
        FlagNonblank = (1 << 0),

        //! Set if the frame is not fully filled with samples from packets.
        //! If this flag is set, frame is partially zero because of lack of packets.
        FlagIncomplete = (1 << 1),

        //! Set if some late packets were dropped while the frame was being built.
        //! It's not necessarty that the frame itself is blank or incomplete.
        FlagDrops = (1 << 2)
    };

    //! Set flags.
    void set_flags(unsigned flags);

    //! Get flags.
    unsigned flags() const;

    //! Get frame data.
    sample_t* samples() const;

    //! Get frame data size.
    size_t num_samples() const;

    //! Print frame to stderr.
    void print() const;

    //! Get ntp timestamp of the 1st sample.
    packet::ntp_timestamp_t ntp_timestamp() const;

    //! Get/set ntp timestamp of the 1st sample.
    packet::ntp_timestamp_t& ntp_timestamp();

    //! Get SampleSpec of the frame.
    const SampleSpec &samplespec() const;

    core::nanoseconds_t duration() const;

private:
    sample_t* samples_;
    size_t num_samples_;
    unsigned flags_;
    packet::ntp_timestamp_t ntp_timestamp_;
    SampleSpec sample_spec_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
