#include "stdafx.h"

#include "SoundRender_TargetA.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"

#include "xrCore/Threading/TaskManager.hpp"

bool CSoundRender_TargetA::_initialize()
{
    A_CHK(alGenBuffers(sdef_target_count, pBuffers));
    alGenSources(1, &pSource);
    const ALenum error = alGetError();
    if (AL_NO_ERROR == error)
    {
        A_CHK(alSourcei(pSource, AL_LOOPING, AL_FALSE));
        A_CHK(alSourcef(pSource, AL_MIN_GAIN, 0.f));
        A_CHK(alSourcef(pSource, AL_MAX_GAIN, 1.f));
        A_CHK(alSourcef(pSource, AL_GAIN, cache_gain));
        A_CHK(alSourcef(pSource, AL_PITCH, cache_pitch));

#ifdef USE_PHONON
        for (const ALuint pBuffer : pBuffers)
        {
            alBufferi(pBuffer, AL_AMBISONIC_LAYOUT_SOFT, AL_ACN_SOFT);
            alBufferi(pBuffer, AL_AMBISONIC_SCALING_SOFT, AL_N3D_SOFT);
        }
#endif
        return true;
    }
    Msg("! sound: OpenAL: Can't create source. Error: %s.", static_cast<pcstr>(alGetString(error)));
    return false;
}

void CSoundRender_TargetA::_destroy()
{
    // clean up target
    if (alIsSource(pSource))
        alDeleteSources(1, &pSource);
    A_CHK(alDeleteBuffers(sdef_target_count, pBuffers));
}

void CSoundRender_TargetA::_restart()
{
    _destroy();
    _initialize();
}

void CSoundRender_TargetA::start(CSoundRender_Emitter* E)
{
    inherited::start(E);

    // Calc storage
    buf_block = E->target_buffer_size;
    g_target_temp_data.resize(buf_block);

    const auto wvf = E->source()->m_wformat;
    const auto blockAlign = wvf.wBitsPerSample / 8 * 4/*wvf.nChannels*/;
    const auto avgBytesPerSec = wvf.nSamplesPerSec * blockAlign;

    g_target_temp_data2.resize(sdef_target_block * avgBytesPerSec / 1000);

#ifdef USE_PHONON
    iplAudioBufferFree(SoundRender->ipl_context, &ipl_buffer_input);
    iplAudioBufferFree(SoundRender->ipl_context, &ipl_buffer_output);
    iplAudioBufferFree(SoundRender->ipl_context, &ipl_buffer_ambi);
    iplAudioBufferFree(SoundRender->ipl_context, &ipl_buffer_mono);
    iplAmbisonicsDecodeEffectRelease(&ipl_decode);

    const IPLint32 samples_per_buf_block = E->ipl_settings.frameSize;

    iplAudioBufferAllocate(SoundRender->ipl_context, wvf.nChannels, samples_per_buf_block,
        &ipl_buffer_input);
    iplAudioBufferAllocate(SoundRender->ipl_context, wvf.nChannels, samples_per_buf_block,
        &ipl_buffer_output);
    iplAudioBufferAllocate(SoundRender->ipl_context, 1, samples_per_buf_block, &ipl_buffer_mono);
    iplAudioBufferAllocate(SoundRender->ipl_context, 2, samples_per_buf_block, &ipl_buffer_stereo);
    iplAudioBufferAllocate(SoundRender->ipl_context, 4, samples_per_buf_block, &ipl_buffer_ambi);

    IPLAmbisonicsDecodeEffectSettings effectSettings
    {
        { IPL_SPEAKERLAYOUTTYPE_STEREO, {}, {} },
        SoundRender->ipl_hrtf, 1
    };

    iplAmbisonicsDecodeEffectCreate(SoundRender->ipl_context, &E->ipl_settings, &effectSettings, &ipl_decode);
#endif
}

void CSoundRender_TargetA::render()
{
    for (size_t i = 0; i < sdef_target_count; ++i)
        fill_block(i);

    for (size_t i = 0; i < sdef_target_count; ++i)
        submit_buffer(pBuffers[i], temp_buf[i].data());

    A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
    A_CHK(alSourcePlay(pSource));

    inherited::render();

    for (size_t i = 0; i < sdef_target_count; ++i)
        buffers_to_prefill.emplace_back(i); // prefill
    TaskScheduler->AddTask("CSoundRender_TargetA::render() - prefill_block", { this, &CSoundRender_TargetA::prefill_block });
}

void CSoundRender_TargetA::stop()
{
    if (rendering)
    {
        A_CHK(alSourceStop(pSource));
        A_CHK(alSourcei(pSource, AL_BUFFER, 0));
        A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, TRUE));
    }
    inherited::stop();
}

void CSoundRender_TargetA::rewind()
{
    inherited::rewind();

    A_CHK(alSourceStop(pSource));
    A_CHK(alSourcei(pSource, AL_BUFFER, 0));

    for (size_t i = 0; i < sdef_target_count; ++i)
        fill_block(i);

    for (size_t i = 0; i < sdef_target_count; ++i)
        submit_buffer(pBuffers[i], temp_buf[i].data());

    A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
    A_CHK(alSourcePlay(pSource));

    for (size_t i = 0; i < sdef_target_count; ++i)
        buffers_to_prefill.emplace_back(i); // prefill
    TaskScheduler->AddTask("CSoundRender_TargetA::rewind() - prefill_block", { this, &CSoundRender_TargetA::prefill_block });
}

void CSoundRender_TargetA::update()
{
    inherited::update();

    ALint processed, state;
    ALenum error;

    /* Get relevant source info */
    alGetSourcei(pSource, AL_SOURCE_STATE, &state);
    alGetSourcei(pSource, AL_BUFFERS_PROCESSED, &processed);
    if ((error = alGetError()) != AL_NO_ERROR)
    {
        Msg("! %s:: source state check failed (0x%d)", __FUNCTION__, error);
        return;
    }

    while (processed > 0)
    {
        ALuint BufferID;
        A_CHK(alSourceUnqueueBuffers(pSource, 1, &BufferID));
        const auto id = get_block_id(BufferID);
        submit_buffer(BufferID, temp_buf[id].data());
        buffers_to_prefill.emplace_back(id);
        A_CHK(alSourceQueueBuffers(pSource, 1, &BufferID));
        processed--;
        if ((error = alGetError()) != AL_NO_ERROR)
        {
            Msg("! %s:: buffering data failed (0x%d)", __FUNCTION__, error);
            return;
        }
    }

    if (!buffers_to_prefill.empty())
        TaskScheduler->AddTask("CSoundRender_TargetA::update() - prefill_block", { this, &CSoundRender_TargetA::prefill_block });

    /* Make sure the source hasn't underrun */
    if (state != AL_PLAYING && state != AL_PAUSED)
    {
        ALint queued;

        /* If no buffers are queued, playback is finished */
        alGetSourcei(pSource, AL_BUFFERS_QUEUED, &queued);
        if (queued == 0)
            return;

        alSourcePlay(pSource);
        if ((error = alGetError()) != AL_NO_ERROR)
        {
            Msg("! %s:: restarting playback failed (0x%d)", __FUNCTION__, error);
            return;
        }
    }
}

void CSoundRender_TargetA::fill_parameters()
{
    [[maybe_unused]] CSoundRender_Emitter* SE = m_pEmitter;
    VERIFY(SE);

    inherited::fill_parameters();

    // 3D params
    VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSourcef(pSource, AL_REFERENCE_DISTANCE, m_pEmitter->p_source.min_distance));

    VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSourcef(pSource, AL_MAX_DISTANCE, m_pEmitter->p_source.max_distance));

    VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSource3f(pSource, AL_POSITION, m_pEmitter->p_source.position.x, m_pEmitter->p_source.position.y,
        -m_pEmitter->p_source.position.z));

    VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSource3f(pSource, AL_VELOCITY, m_pEmitter->p_source.velocity.x, m_pEmitter->p_source.velocity.y,
        -m_pEmitter->p_source.velocity.z));

    VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, m_pEmitter->b2D));

    A_CHK(alSourcef(pSource, AL_ROLLOFF_FACTOR, psSoundRolloff));

    VERIFY2(m_pEmitter, SE->source()->file_name());
    float _gain = m_pEmitter->smooth_volume;
    clamp(_gain, EPS_S, 1.f);
    if (!fsimilar(_gain, cache_gain, 0.01f))
    {
        cache_gain = _gain;
        A_CHK(alSourcef(pSource, AL_GAIN, _gain));
    }

    VERIFY2(m_pEmitter, SE->source()->file_name());

    float _pitch = m_pEmitter->p_source.freq;
    if (!m_pEmitter->bIgnoringTimeFactor)
        _pitch *= psSoundTimeFactor; //--#SM+#-- Correct sound "speed" by time factor
    clamp(_pitch, EPS_L, 100.f); //--#SM+#-- Increase sound frequency (speed) limit

    if (!fsimilar(_pitch, cache_pitch))
    {
        cache_pitch = _pitch;
        A_CHK(alSourcef(pSource, AL_PITCH, _pitch));
    }
    VERIFY2(m_pEmitter, SE->source()->file_name());
}

size_t CSoundRender_TargetA::get_block_id(ALuint BufferID) const
{
    const auto it = std::find(std::begin(pBuffers), std::end(pBuffers), BufferID);
    return it - std::begin(pBuffers);
}

void CSoundRender_TargetA::submit_buffer(ALuint BufferID, const void* data) const
{
    R_ASSERT(m_pEmitter);

    const auto& wvf = m_pEmitter->source()->m_wformat;
    const bool mono = wvf.nChannels == 1;

#ifdef USE_PHONON
    if (psSoundFlags.test(ss_EFX) && m_pEmitter->scene->ipl_scene_mesh && !m_pEmitter->is_2D())
    {
        IPLSimulationOutputs outputs{};
        iplSourceGetOutputs(m_pEmitter->ipl_source,
            static_cast<IPLSimulationFlags>(IPL_SIMULATIONFLAGS_DIRECT | IPL_SIMULATIONFLAGS_REFLECTIONS),
            &outputs);

        iplAudioBufferDeinterleave(SoundRender->ipl_context, (float*)g_target_temp_data.data(), &ipl_buffer_input);

        iplDirectEffectApply(m_pEmitter->ipl_effects.direct, &outputs.direct, &ipl_buffer_input, &ipl_buffer_output);

        const IPLCoordinateSpace3 listenerCoordinates
        {
            { 1, 0, 0 },
            { 0, 1, 0 },
            { 0, 0, 1 },
            reinterpret_cast<const IPLVector3&>(SoundRender->listener_position())
        }; // the world-space position and orientation of the listener

        //outputs.pathing.order = 1;
        //outputs.pathing.binaural = IPL_TRUE;
        //outputs.pathing.hrtf = SoundRender->ipl_hrtf;
        //outputs.pathing.listener = listenerCoordinates;

        if (!mono)
            iplAudioBufferDownmix(SoundRender->ipl_context, &ipl_buffer_output, &ipl_buffer_mono);

        //iplPathEffectApply(ipl_path_effect, &outputs.pathing, mono ? &ipl_buffer_output : &ipl_buffer_mono, &ipl_buffer_ambi);

        iplReflectionEffectApply(m_pEmitter->ipl_effects.reflection,
            &outputs.reflections,
            mono ? &ipl_buffer_output : &ipl_buffer_mono, &ipl_buffer_ambi,
            nullptr);

        IPLAmbisonicsDecodeEffectParams params
        {
            1, SoundRender->ipl_hrtf, listenerCoordinates, IPL_TRUE
        };

        iplAmbisonicsDecodeEffectApply(ipl_decode, &params, &ipl_buffer_ambi, &ipl_buffer_stereo);
        if (mono)
            iplAudioBufferDownmix(SoundRender->ipl_context, &ipl_buffer_stereo, &ipl_buffer_mono);

        iplAudioBufferMix(SoundRender->ipl_context, mono ? &ipl_buffer_mono : &ipl_buffer_stereo, &ipl_buffer_output);

        iplAudioBufferInterleave(SoundRender->ipl_context, &ipl_buffer_output, (float*)g_target_temp_data.data());
    }
#endif

    ALuint format;
    if (wvf.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        format = mono ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;
    else
    {
        format = mono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    }

    A_CHK(alBufferData(BufferID, format, data, buf_block, m_pEmitter->source()->m_wformat.nSamplesPerSec));
}
