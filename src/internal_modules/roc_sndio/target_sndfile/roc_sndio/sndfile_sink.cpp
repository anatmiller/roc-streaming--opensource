/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sndfile_sink.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"
#include "roc_sndio/sndfile_extension_table.h"

namespace roc {
namespace sndio {

namespace {

bool map_to_sub_format(SF_INFO& file_info_, int format_enum) {
    // Provides the minimum number of sub formats needed to support all
    // possible major formats
    int high_to_low_sub_formats[] = {
        SF_FORMAT_PCM_24,
        SF_FORMAT_PCM_16,
        SF_FORMAT_DPCM_16,
    };

    for (size_t format_attempt = 0;
         format_attempt < ROC_ARRAY_SIZE(high_to_low_sub_formats); format_attempt++) {
        file_info_.format = format_enum | high_to_low_sub_formats[format_attempt];

        if (sf_format_check(&file_info_)) {
            return true;
        }
    }

    return false;
}

bool map_to_sndfile(const char** driver, const char* path, SF_INFO& file_info_) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    const char* file_extension = NULL;
    const char* dot = strrchr(path, '.');

    if (!dot || dot == path) {
        return false;
    }

    file_extension = dot + 1;

    int format_enum = 0;

    // First try to select format by iterating through file_map_index.
    if (*driver == NULL) {
        // If driver is NULL, match by file extension.
        for (size_t file_map_index = 0; file_map_index < ROC_ARRAY_SIZE(file_type_map);
             file_map_index++) {
            if (file_type_map[file_map_index].file_extension != NULL) {
                if (strcmp(file_extension, file_type_map[file_map_index].file_extension)
                    == 0) {
                    format_enum = file_type_map[file_map_index].format_id;
                    *driver = file_extension;
                    break;
                }
            }
        }
    } else {
        // If driver is non-NULL, match by driver name.
        for (size_t file_map_index = 0; file_map_index < ROC_ARRAY_SIZE(file_type_map);
             file_map_index++) {
            if (strcmp(*driver, file_type_map[file_map_index].driver_name) == 0) {
                format_enum = file_type_map[file_map_index].format_id;
                break;
            }
        }
    }

    // Then try to select format by iterating through all sndfile formats.
    if (format_enum == 0) {
        SF_FORMAT_INFO info;
        int major_count = 0, format_index = 0;
        if (int errnum =
                sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &major_count, sizeof(int))) {
            roc_panic(
                "sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR_COUNT) failed: %s",
                sf_error_number(errnum));
        }

        for (format_index = 0; format_index < major_count; format_index++) {
            info.format = format_index;
            if (int errnum =
                    sf_command(NULL, SFC_GET_FORMAT_MAJOR, &info, sizeof(info))) {
                roc_panic("sndfile backend: sf_command(SFC_GET_FORMAT_MAJOR) failed: %s",
                          sf_error_number(errnum));
            }

            if (*driver == NULL) {
                // If driver is NULL, match by file extension.
                if (strcmp(info.extension, file_extension) == 0) {
                    format_enum = info.format;
                    *driver = file_extension;
                    break;
                }
            } else {
                // If driver is non-NULL, match by driver name.
                if (strcmp(info.extension, *driver) == 0) {
                    format_enum = info.format;
                    break;
                }
            }
        }
    }

    if (format_enum == 0) {
        return false;
    }

    roc_log(LogDebug, "detected file format type '%s'", *driver);

    file_info_.format = format_enum | file_info_.format;

    if (sf_format_check(&file_info_)) {
        // Format is supported as is.
        return true;
    } else {
        // Format may be supported if combined with a subformat.
        return map_to_sub_format(file_info_, format_enum);
    }
}

} // namespace

SndfileSink::SndfileSink(audio::FrameFactory& frame_factory,
                         core::IArena& arena,
                         const Config& config)
    : file_(NULL)
    , init_status_(status::NoStatus) {
    if (config.latency != 0) {
        roc_log(LogError,
                "sndfile sink: setting io latency not supported by sndfile backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    sample_spec_ = config.sample_spec;

    sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                              audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                              44100);

    // TODO(gh-696): map format from sample_spec
    if (!sample_spec_.is_raw()) {
        roc_log(LogError, "sndfile sink: sample format can be only \"-\" or \"%s\"",
                audio::pcm_format_to_str(audio::Sample_RawFormat));
        init_status_ = status::StatusBadConfig;
        return;
    }

    memset(&file_info_, 0, sizeof(file_info_));

    file_info_.format = SF_FORMAT_PCM_32;
    file_info_.channels = (int)sample_spec_.num_channels();
    file_info_.samplerate = (int)sample_spec_.sample_rate();

    init_status_ = status::StatusOK;
}

SndfileSink::~SndfileSink() {
    if (file_) {
        roc_panic("sndfile sink: output file is not closed");
    }
}

status::StatusCode SndfileSink::close() {
    return close_();
}

status::StatusCode SndfileSink::init_status() const {
    return init_status_;
}

status::StatusCode SndfileSink::open(const char* driver, const char* path) {
    roc_log(LogDebug, "sndfile sink: opening: driver=%s path=%s", driver, path);

    if (file_) {
        roc_panic("sndfile sink: can't call open() more than once");
    }

    if (!path) {
        roc_panic("sndfile sink: path is null");
    }

    return open_(driver, path);
}

DeviceType SndfileSink::type() const {
    return DeviceType_Sink;
}

ISink* SndfileSink::to_sink() {
    return this;
}

ISource* SndfileSink::to_source() {
    return NULL;
}

audio::SampleSpec SndfileSink::sample_spec() const {
    if (!file_) {
        roc_panic("sndfile sink: not opened");
    }

    return sample_spec_;
}

bool SndfileSink::has_state() const {
    return false;
}

bool SndfileSink::has_latency() const {
    return false;
}

bool SndfileSink::has_clock() const {
    return false;
}

status::StatusCode SndfileSink::write(audio::Frame& frame) {
    if (!file_) {
        roc_panic("sndfile sink: not opened");
    }

    audio::sample_t* frame_data = frame.raw_samples();
    sf_count_t frame_size = (sf_count_t)frame.num_raw_samples();

    // Write entire float buffer in one call
    const sf_count_t count = sf_write_float(file_, frame_data, frame_size);
    const int err = sf_error(file_);

    if (count != frame_size || err != 0) {
        roc_log(LogError, "sndfile source: sf_write_float() failed: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    return status::StatusOK;
}

status::StatusCode SndfileSink::open_(const char* driver, const char* path) {
    if (!map_to_sndfile(&driver, path, file_info_)) {
        roc_log(LogDebug,
                "sndfile sink: map_to_sndfile():"
                " cannot find valid subtype format for major format type");
        return status::StatusErrFile;
    }

    file_ = sf_open(path, SFM_WRITE, &file_info_);
    if (!file_) {
        roc_log(LogDebug, "sndfile sink: %s, can't open: driver=%s path=%s",
                sf_strerror(file_), driver, path);
        return status::StatusErrFile;
    }

    if (sf_command(file_, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) == 0) {
        roc_log(LogDebug,
                "sndfile sink: sf_command(SFC_SET_UPDATE_HEADER_AUTO) returned false");
        return status::StatusErrFile;
    }

    sample_spec_.set_sample_rate((size_t)file_info_.samplerate);

    roc_log(LogInfo, "sndfile sink: opened: %s",
            audio::sample_spec_to_str(sample_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode SndfileSink::close_() {
    if (!file_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "sndfile sink: closing output");

    const int err = sf_close(file_);
    if (err != 0) {
        roc_log(LogError,
                "sndfile sink: sf_close() failed, cannot properly close output: %s",
                sf_error_number(err));
        return status::StatusErrFile;
    }

    file_ = NULL;

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
