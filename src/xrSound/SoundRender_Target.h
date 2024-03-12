#pragma once

#include "SoundRender.h"

class Task;

class CSoundRender_Target
{
protected:
    CSoundRender_Emitter* m_pEmitter{};
    bool rendering{};

    u32 buf_block{};
    xr_vector<u8> temp_buf[sdef_target_count];
    void fill_block(size_t idx);
    void fill_all_blocks();

    xr_vector<size_t> buffers_to_prefill;
    void prefill_blocks(Task&, void*);
    void prefill_all_blocks(Task&, void*);

    std::atomic<Task*> prefill_task{};
    void dispatch_prefill();
    void dispatch_prefill_all();

public:
    void wait_prefill() const;

public:
    float priority{};

public:
    CSoundRender_Target();
    virtual ~CSoundRender_Target() = default;

    CSoundRender_Emitter* get_emitter() const { return m_pEmitter; }
    bool get_Rendering() const { return rendering; }

    virtual bool _initialize() = 0;
    virtual void _destroy();
    virtual void _restart() = 0;

    virtual void start(CSoundRender_Emitter* E);
    virtual void render();
    virtual void rewind();
    virtual void stop();
    virtual void update();
    virtual void fill_parameters();
};
