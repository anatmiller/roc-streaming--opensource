/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iresampler.h
//! @brief Audio resampler interface.

#ifndef ROC_AUDIO_IRESAMPLER_H_
#define ROC_AUDIO_IRESAMPLER_H_

#include "roc_audio/frame.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

//! Audio writer interface.
class IResampler {
public:
    virtual ~IResampler();

    //! Check if object is successfully constructed.
    virtual bool valid() const = 0;

    //! Set new resample factor.
    //! @remarks
    //!  Resampling algorithm needs some window of input samples. The length of the window
    //!  (length of sinc impulse response) is a compromise between SNR and speed. It
    //!  depends on current resampling factor. So we choose length of input buffers to let
    //!  it handle maximum length of input. If new scaling factor breaks equation this
    //!  function returns false.
    virtual bool set_scaling(float) = 0;

    //! Resamples the whole output frame.
    virtual bool resample_buff(Frame& out) = 0;

    //! Push new buffer on the front of the internal FIFO, which comprisesthree window_.
    virtual void renew_buffers(core::Slice<sample_t>& prev,
                               core::Slice<sample_t>& cur,
                               core::Slice<sample_t>& next) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IRESAMPLER_H_
