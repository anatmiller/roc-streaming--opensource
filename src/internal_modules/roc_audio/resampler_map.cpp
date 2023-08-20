/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_builtin.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_ptr.h"

#ifdef ROC_TARGET_SPEEXDSP
#include "roc_audio/resampler_speex.h"
#endif // ROC_TARGET_SPEEXDSP

namespace roc {
namespace audio {

namespace {

template <class T>
IResampler* resampler_ctor(core::IArena& arena,
                           core::BufferFactory<sample_t>& buffer_factory,
                           ResamplerProfile profile,
                           const audio::SampleSpec& in_spec,
                           const audio::SampleSpec& out_spec) {
    return new (arena) T(arena, buffer_factory, profile, in_spec, out_spec);
}

} // namespace

ResamplerMap::ResamplerMap()
    : n_backends_(0) {
#ifdef ROC_TARGET_SPEEXDSP
    {
        Backend back;
        back.id = ResamplerBackend_Speex;
        back.ctor = &resampler_ctor<SpeexResampler>;
        add_backend_(back);
    }
#endif // ROC_TARGET_SPEEXDSP
    {
        Backend back;
        back.id = ResamplerBackend_Builtin;
        back.ctor = &resampler_ctor<BuiltinResampler>;
        add_backend_(back);
    }
}

size_t ResamplerMap::num_backends() const {
    return n_backends_;
}

ResamplerBackend ResamplerMap::nth_backend(size_t n) const {
    roc_panic_if_not(n < n_backends_);
    return backends_[n].id;
}

bool ResamplerMap::is_supported(ResamplerBackend backend_id) const {
    return find_backend_(backend_id) != NULL;
}

IResampler* ResamplerMap::new_resampler(ResamplerBackend backend_id,
                                        core::IArena& arena,
                                        core::BufferFactory<sample_t>& buffer_factory,
                                        ResamplerProfile profile,
                                        const audio::SampleSpec& in_spec,
                                        const audio::SampleSpec& out_spec) {
    const Backend* backend = find_backend_(backend_id);
    if (!backend) {
        roc_log(LogError, "resampler map: unsupported resampler backend: [%d] %s",
                backend_id, resampler_backend_to_str(backend_id));
        return NULL;
    }

    core::ScopedPtr<IResampler> resampler(
        backend->ctor(arena, buffer_factory, profile, in_spec, out_spec), arena);

    if (!resampler || !resampler->is_valid()) {
        return NULL;
    }

    return resampler.release();
}

void ResamplerMap::add_backend_(const Backend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = backend;
}

const ResamplerMap::Backend*
ResamplerMap::find_backend_(ResamplerBackend backend_id) const {
    if (backend_id == ResamplerBackend_Default) {
        roc_panic_if(n_backends_ == 0);
        return &backends_[0];
    }
    for (size_t n = 0; n < n_backends_; n++) {
        if (backends_[n].id == backend_id) {
            return &backends_[n];
        }
    }
    return NULL;
}

} // namespace audio
} // namespace roc
