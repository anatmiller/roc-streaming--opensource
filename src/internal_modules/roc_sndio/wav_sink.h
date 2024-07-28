/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/wav_sink.h
//! @brief WAV sink.

#ifndef ROC_SNDIO_WAV_SINK_H_
#define ROC_SNDIO_WAV_SINK_H_

#include "roc_audio/frame_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/wav_header.h"

namespace roc {
namespace sndio {

//! WAV sink.
//! @remarks
//!  Writes samples to output WAV file.
class WavSink : public ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    WavSink(audio::FrameFactory& frame_factory,
            core::IArena& arena,
            const Config& config);
    ~WavSink();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Open sink.
    ROC_ATTR_NODISCARD status::StatusCode open(const char* device);

    //! Get device type.
    virtual DeviceType type() const;

    //! Try to cast to ISink.
    virtual ISink* to_sink();

    //! Try to cast to ISource.
    virtual ISource* to_source();

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Check if the sink supports state updates.
    virtual bool has_state() const;

    //! Check if the sink supports latency reports.
    virtual bool has_latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Explicitly close the sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Write frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(audio::Frame& frame);

private:
    status::StatusCode open_(const char* path);
    status::StatusCode close_();

    audio::SampleSpec sample_spec_;

    FILE* output_file_;
    core::Optional<WavHeader> header_;

    status::StatusCode init_status_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WAV_SINK_H_
