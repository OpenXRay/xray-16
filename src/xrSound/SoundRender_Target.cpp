#include "stdafx.h"

#include "SoundRender_Target.h"
#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"

void CSoundRender_Target::start(CSoundRender_Emitter* E)
{
    R_ASSERT(E);

    // *** Initial buffer startup ***
    // 1. Fill parameters
    // 4. Load 2 blocks of data (as much as possible)
    // 5. Deferred-play-signal (emitter-exist, rendering-false)
    m_pEmitter = E;
    rendering = false;
    //m_pEmitter->source()->attach();

    // Calc storage
    buf_block = sdef_target_block * E->source()->m_wformat.nAvgBytesPerSec / 1000;
    for (auto& buf : temp_buf)
        buf.resize(buf_block);
}

void CSoundRender_Target::render() { rendering = true; }
void CSoundRender_Target::stop()
{
    m_pEmitter->source()->detach();
    m_pEmitter = nullptr;
    rendering = false;
}

void CSoundRender_Target::rewind() { R_ASSERT(rendering); }
void CSoundRender_Target::update() { R_ASSERT(m_pEmitter); }
void CSoundRender_Target::fill_parameters()
{
    VERIFY(m_pEmitter);
    //if (pEmitter->b2D)
    //    pEmitter->set_position(SoundRender->listener_position());
}

void CSoundRender_Target::fill_block(size_t idx)
{
    R_ASSERT(m_pEmitter);
    m_pEmitter->fill_block(temp_buf[idx].data(), buf_block);
}
