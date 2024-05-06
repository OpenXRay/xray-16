#include "stdafx.h"

#include "SoundRender_TargetA.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"

CSoundRender_TargetA::CSoundRender_TargetA()
{
}

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
        return true;
    }
    Msg("! sound: OpenAL: Can't create source. Error: %s.", static_cast<pcstr>(alGetString(error)));
    return false;
}

void CSoundRender_TargetA::_destroy()
{
    CSoundRender_Target::_destroy();
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

void CSoundRender_TargetA::render()
{
    inherited::render();

    submit_all_buffers();

    A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
    A_CHK(alSourcePlay(pSource));

    dispatch_prefill_all();
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

    fill_all_blocks();
    submit_all_buffers();

    A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
    A_CHK(alSourcePlay(pSource));

    dispatch_prefill_all();
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
        submit_buffer(BufferID, temp_buf[id].data(), temp_buf[id].size());

        A_CHK(alSourceQueueBuffers(pSource, 1, &BufferID));
        processed--;
        if ((error = alGetError()) != AL_NO_ERROR)
        {
            Msg("! %s:: buffering data failed (0x%d)", __FUNCTION__, error);
            return;
        }
        buffers_to_prefill.emplace_back(id);
    }

    if (!buffers_to_prefill.empty())
        dispatch_prefill();

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

void CSoundRender_TargetA::submit_buffer(ALuint BufferID, const void* data, size_t dataSize) const
{
    R_ASSERT1_CURE(m_pEmitter, true, { return; });

    const auto& info = m_pEmitter->source()->data_info();
    const bool mono = info.channels == 1;

    ALuint format;
    if (info.format == SoundFormat::Float32)
        format = mono ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;
    else
    {
        format = mono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    }

    A_CHK(alBufferData(BufferID, format, data, dataSize, info.samplesPerSec));
}

void CSoundRender_TargetA::submit_all_buffers() const
{
    for (size_t i = 0; i < sdef_target_count; ++i)
        submit_buffer(pBuffers[i], temp_buf[i].data(), temp_buf[i].size());
}
