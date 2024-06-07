#pragma once

#include "SoundRender.h"

class CSoundRender_Target
{
protected:
    CSoundRender_Emitter* m_pEmitter{};
    bool rendering{};
    float priority{ -1 };

public:
    CSoundRender_Target() = default;
    virtual ~CSoundRender_Target() = default;

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

    ICF auto get_priority() const { return priority; }
    ICF void set_priority(const float p) { priority = p; }
};
