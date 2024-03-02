#pragma once

#include "SoundRender.h"

class CSoundRender_Target
{
protected:
    CSoundRender_Emitter* m_pEmitter{};
    bool rendering{};

public:
    float priority{};

protected:
    OggVorbis_File ovf{};
    IReader* wave{};
    void attach();
    void detach();

public:
    OggVorbis_File* get_data()
    {
        if (!wave)
            attach();
        return &ovf;
    }

    CSoundRender_Target() = default;
    virtual ~CSoundRender_Target();

    CSoundRender_Emitter* get_emitter() const { return m_pEmitter; }
    bool get_Rendering() const { return rendering; }

    virtual bool _initialize() = 0;
    virtual void _destroy() = 0;
    virtual void _restart() = 0;

    virtual void start(CSoundRender_Emitter* E);
    virtual void render();
    virtual void rewind();
    virtual void stop();
    virtual void update();
    virtual void fill_parameters();
};
