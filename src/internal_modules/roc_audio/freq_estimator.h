/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/freq_estimator.h
//! @brief Frequency estimator.

#ifndef ROC_AUDIO_FREQ_ESTIMATOR_H_
#define ROC_AUDIO_FREQ_ESTIMATOR_H_

#include "roc_audio/freq_estimator_decim.h"
#include "roc_audio/sample.h"
#include "roc_core/csv_dumper.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! FreqEstimator paremeter preset.
enum FreqEstimatorProfile {
    //! Fast and responsive tuning.
    //! Good for lower network latency and jitter.
    FreqEstimatorProfile_Responsive,

    //! Slow and smooth tuning.
    //! Good for higher network latency and jitter.
    FreqEstimatorProfile_Gradual
};

//! FreqEstimator tunable parameters.
struct FreqEstimatorConfig {
    double P; //!< Proportional gain of PI-controller.
    double I; //!< Integral gain of PI-controller.

    //! How much downsample input value (latency buffer size) on the first stage.
    //! Must be less or equal to fe_decim_factor_max and must be greater than zero.
    size_t decimation_factor1;

    //! How much downsample input value on the second stage. Must be less or equal
    //! to fe_decim_factor_max. Could be zero to disable the second decimation stage.
    size_t decimation_factor2;

    //! Within this range we consider the FreqEstimator is stable.
    //! stable_criteria > error / target;
    double stable_criteria;

    //! How much time current latency readings must be within stable_criteria range
    //! to let FreqEstimator switch into stable state.
    core::nanoseconds_t stability_duration_criteria;

    //! FreqEstimator limits its output control action value with this value so as to
    //! keep sensible pace of latency adjustment if there is a long way to go.
    double control_action_saturation_cap;

    FreqEstimatorConfig()
        : P(0)
        , I(0)
        , decimation_factor1(0)
        , decimation_factor2(0)
        , stable_criteria(0.1)
        , stability_duration_criteria(0)
        , control_action_saturation_cap(0) {
    }
};

//! Evaluates sender's frequency to receivers's frequency ratio.
//! @remarks
//!  We provide FreqEstimator with traget latency and periodically update it with
//!  the actual latency. In response, FreqEstimator computes frequency coefficient,
//!  the ratio of sender to receiver frequency. This coefficient is then set as
//!  the scaling factor of the resampler, which in result compensates the frequency
//!  difference and moves the latency closer to its target value.
class FreqEstimator : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p profile defines configuration preset.
    //!  - @p target_latency defines latency we want to archive.
    FreqEstimator(FreqEstimatorProfile profile,
                  packet::stream_timestamp_t target_latency,
                  roc::core::CsvDumper* dumper);

    //! Get current frequecy coefficient.
    float freq_coeff() const;

    //! Compute new value of frequency coefficient.
    void update_current_latency(packet::stream_timestamp_t current_latency);

    //! Update target latency.
    void update_target_latency(packet::stream_timestamp_t target_latency);

    //! Is FreqEstimator in stable state.
    //! @remarks
    //!  If current_latency is in kept within certain limits around target_latency
    //!  FreqEstimator is in 'stable' state, otherwise it is 'not-stable' state.
    //!  The state affects internal regulator strategy and it effectiveness.
    bool is_stable() const;

private:
    bool run_decimators_(packet::stream_timestamp_t current, double& filtered);
    double run_controller_(double current);
    void dump_(double filtered);

    const FreqEstimatorConfig config_;
    double target_; // Target latency.

    double dec1_casc_buff_[fe_decim_len];
    size_t dec1_ind_;

    double dec2_casc_buff_[fe_decim_len];
    size_t dec2_ind_;

    size_t samples_counter_; // Input samples counter.
    double accum_;           // Integrator value.

    double coeff_; // Current frequency coefficient value.

    bool stable_; // True if FreqEstimator has stabilized.
    // Last time when FreqEstimator was out of range.
    core::nanoseconds_t last_unstable_time_;

    core::CsvDumper* dumper_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FREQ_ESTIMATOR_H_
