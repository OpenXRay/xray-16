#ifndef xrTheora_StreamH
#define xrTheora_StreamH
#pragma once

#include <theora/theora.h>
#include "xrCore/stream_reader.h"

class ENGINE_API CTheoraStream
{
    friend class CTheoraSurface;

    ogg_sync_state o_sync_state;
    ogg_page o_page;
    ogg_stream_state o_stream_state;
    theora_info t_info;
    theora_comment t_comment;
    theora_state t_state;

#ifdef _EDITOR
    IReader* source;
#else
    CStreamReader* source;
#endif
    yuv_buffer t_yuv_buffer;

    ogg_int64_t d_frame;
    u32 tm_total;
    u32 key_rate; // theora have const key rate
    float fpms;

protected:
    int ReadData();
    BOOL ParseHeaders();

public:
    CTheoraStream();
    virtual ~CTheoraStream();

    BOOL Load(const char* fname);

    void Reset();

    BOOL Decode(u32 tm_play);

    yuv_buffer* CurrentFrame() { return &t_yuv_buffer; }
};

#endif // xrTheora_StreamH
