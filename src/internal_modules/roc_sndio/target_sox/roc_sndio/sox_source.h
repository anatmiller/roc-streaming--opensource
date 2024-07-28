/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_source.h
//! @brief SoX source.

#ifndef ROC_SNDIO_SOX_SOURCE_H_
#define ROC_SNDIO_SOX_SOURCE_H_

#include <sox.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/string_buffer.h"
#include "roc_sndio/config.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! SoX source.
//! @remarks
//!  Reads samples from input file or device.
//!  Supports multiple drivers for different file types and audio systems.
class SoxSource : public ISource, private core::NonCopyable<> {
public:
    //! Initialize.
    SoxSource(audio::FrameFactory& frame_factory,
              core::IArena& arena,
              const Config& config,
              DriverType driver_type);
    ~SoxSource();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Open sink.
    ROC_ATTR_NODISCARD status::StatusCode open(const char* driver, const char* path);

    //! Get device type.
    virtual DeviceType type() const;

    //! Try to cast to ISink.
    virtual ISink* to_sink();

    //! Try to cast to ISource.
    virtual ISource* to_source();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Check if the source supports state updates.
    virtual bool has_state() const;

    //! Get source state.
    virtual DeviceState state() const;

    //! Pause source.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume source.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Explicitly close the source.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Restart reading from beginning.
    virtual ROC_ATTR_NODISCARD status::StatusCode rewind();

    //! Adjust source clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t timestamp);

    //! Read frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(audio::Frame& frame,
         packet::stream_timestamp_t duration,
         audio::FrameReadMode mode);

private:
    status::StatusCode init_names_(const char* driver, const char* path);
    status::StatusCode init_buffer_();

    status::StatusCode open_();
    status::StatusCode seek_(uint64_t offset);
    status::StatusCode close_();

    audio::FrameFactory& frame_factory_;

    const DriverType driver_type_;
    core::StringBuffer driver_name_;
    core::StringBuffer input_name_;

    core::Array<sox_sample_t> buffer_;
    size_t buffer_size_;
    core::nanoseconds_t frame_length_;
    audio::SampleSpec sample_spec_;

    sox_format_t* input_;
    sox_signalinfo_t in_signal_;

    bool eof_;
    bool paused_;

    status::StatusCode init_status_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_SOURCE_H_
