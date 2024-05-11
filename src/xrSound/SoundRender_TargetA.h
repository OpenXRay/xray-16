#pragma once

#include "SoundRender_Target.h"
#include "SoundRender_CoreA.h"

class CSoundRender_TargetA : public CSoundRender_Target
{
    using inherited = CSoundRender_Target;

    // OpenAL
    ALuint pSource{};
    ALuint pBuffers[sdef_target_count]{};
    ALuint dataFormat;
    ALsizei sampleRate;

    float cache_gain{};
    float cache_pitch{ 1.0f };

    void submit_buffer(ALuint BufferID) const;
    void submit_all_buffers() const;

public:
    CSoundRender_TargetA();

    bool _initialize() override;
    void _destroy() override;
    void _restart() override;

    void start(CSoundRender_Emitter* E) override;
    void render() override;
    void rewind() override;
    void stop() override;
    void update() override;
    void fill_parameters() override;
};
