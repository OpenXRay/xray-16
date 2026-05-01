////////////////////////////////////////////////////////////////////////////
//	Created		: 29.07.2009
//	Author		: Armen Abroyan
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stats_manager.h"

namespace xray::render::fg
{
#if defined(USE_DX11)
static u32 get_format_pixel_size(DXGI_FORMAT format);
#endif

void stats_manager::increment_stats(u32 size, enum_stats_buffer_type type, D3D_POOL location)
{
    if (GEnv.isDedicatedServer)
        return;

    R_ASSERT(type >= 0 && type < enum_stats_buffer_type_COUNT);
    R_ASSERT(location >= 0 && location <= D3D_POOL_SCRATCH);
    memory_usage_summary[type][location] += size;
}

void stats_manager::increment_stats(u32 size, enum_stats_buffer_type type, D3D_POOL location, void* buff_ptr)
{
    if (GEnv.isDedicatedServer)
        return;

    R_ASSERT(buff_ptr != NULL);
    R_ASSERT(type >= 0 && type < enum_stats_buffer_type_COUNT);
    R_ASSERT(location >= 0 && location <= D3D_POOL_SCRATCH);
    memory_usage_summary[type][location] += size;

#ifdef DEBUG
    stats_item new_item;
    new_item.buff_ptr = buff_ptr;
    new_item.location = location;
    new_item.type = type;
    new_item.size = size;

    m_buffers_list.push_back(new_item);
#endif
}

void stats_manager::increment_stats_rtarget(ID3DTexture2D* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

    D3D_POOL pool = D3D_POOL_DEFAULT;
#if defined(USE_DX11)
    D3D_TEXTURE2D_DESC desc;
    buff->GetDesc(&desc);
#else
#   error No graphics API selected or enabled!
#endif

    u32 size = desc.Height * desc.Width * get_format_pixel_size(desc.Format);
    increment_stats(size, enum_stats_buffer_type_rtarget, pool, buff);
}

void stats_manager::increment_stats_vb(ID3DVertexBuffer* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

#if defined(USE_DX11)
    D3D_BUFFER_DESC desc;
    buff->GetDesc(&desc);
    increment_stats(desc.ByteWidth, enum_stats_buffer_type_vertex, D3D_POOL_MANAGED, buff);
#else
#   error No graphics API selected or enabled!
#endif
}

void stats_manager::increment_stats_ib(ID3DIndexBuffer* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

#if defined(USE_DX11)
    D3D_BUFFER_DESC desc;
    buff->GetDesc(&desc);
    increment_stats(desc.ByteWidth, enum_stats_buffer_type_index, D3D_POOL_MANAGED, buff);
#else
#   error No graphics API selected or enabled!
#endif
}

void stats_manager::decrement_stats_rtarget(ID3DTexture2D* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

    buff->AddRef();
    int refcnt = 0;
    if ((refcnt = buff->Release()) > 1)
        return;

    D3D_POOL pool = D3D_POOL_DEFAULT;
#if defined(USE_DX11)
    D3D_TEXTURE2D_DESC desc;
    buff->GetDesc(&desc);
#else
#   error No graphics API selected or enabled!
#endif

    u32 size = desc.Height * desc.Width * get_format_pixel_size(desc.Format);
    decrement_stats(size, enum_stats_buffer_type_rtarget, pool, buff);
}

void stats_manager::decrement_stats_vb(ID3DVertexBuffer* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

    buff->AddRef();
    int refcnt = 0;
    if ((refcnt = buff->Release()) > 1)
        return;

#if defined(USE_DX11)
    D3D_BUFFER_DESC desc;
    buff->GetDesc(&desc);
    decrement_stats(desc.ByteWidth, enum_stats_buffer_type_vertex, D3D_POOL_MANAGED, buff);
#else
#   error No graphics API selected or enabled!
#endif
}

void stats_manager::decrement_stats_ib(ID3DIndexBuffer* buff)
{
    if (buff == nullptr || GEnv.isDedicatedServer)
        return;

    buff->AddRef();
    int refcnt = 0;
    if ((refcnt = buff->Release()) > 1)
        return;

#if defined(USE_DX11)
    D3D_BUFFER_DESC desc;
    buff->GetDesc(&desc);
    decrement_stats(desc.ByteWidth, enum_stats_buffer_type_index, D3D_POOL_MANAGED, buff);
#else
#   error No graphics API selected or enabled!
#endif
}

void stats_manager::decrement_stats(u32 size, enum_stats_buffer_type type, D3D_POOL location)
{
    if (GEnv.isDedicatedServer)
        return;

    R_ASSERT(type >= 0 && type < enum_stats_buffer_type_COUNT);
    R_ASSERT(location >= 0 && location <= D3D_POOL_SCRATCH);
    memory_usage_summary[type][location] -= size;
}

void stats_manager::decrement_stats(u32 size, enum_stats_buffer_type type, D3D_POOL location, void* buff_ptr)
{
    if (buff_ptr == nullptr || GEnv.isDedicatedServer)
        return;

#ifdef DEBUG
    xr_vector<stats_item>::iterator it = m_buffers_list.begin();
    xr_vector<stats_item>::const_iterator en = m_buffers_list.end();
    bool find = false;
    for (; it != en; ++it)
    {
        if (it->buff_ptr == buff_ptr)
        {
            // The pointers may coincide so this assertion may some times fail normally.
            // R_ASSERT ( it->type == type && it->location == location && it->size == size );
            m_buffers_list.erase(it);
            find = true;
            break;
        }
    }
    R_ASSERT(find); //  "Specified buffer not fount in the buffers list.
//	The buffer may not incremented to stats or it already was removed"
#endif // DEBUG

    memory_usage_summary[type][location] -= size;
}

stats_manager::~stats_manager()
{
#ifdef DEBUG
    Msg("m_buffers_list.size() = %d", m_buffers_list.size());
//	R_ASSERT( m_buffers_list.size() == 0);	//  Some buffers stats are not removed from the list.
#endif
}

#if defined(USE_DX11)
static u32 get_format_pixel_size(DXGI_FORMAT format)
{
    if (format >= DXGI_FORMAT_R32G32B32A32_TYPELESS && format <= DXGI_FORMAT_R32G32B32A32_SINT)
        return 16;
    else if (format >= DXGI_FORMAT_R32G32B32_TYPELESS && format <= DXGI_FORMAT_R32G32B32_SINT)
        return 12;
    else if (format >= DXGI_FORMAT_R16G16B16A16_TYPELESS && format <= DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
        return 8;
    else if (format >= DXGI_FORMAT_R10G10B10A2_TYPELESS && format <= DXGI_FORMAT_X24_TYPELESS_G8_UINT)
        return 4;
    else if (format >= DXGI_FORMAT_R8G8_TYPELESS && format <= DXGI_FORMAT_R16_SINT)
        return 2;
    else if (format >= DXGI_FORMAT_R8_TYPELESS && format <= DXGI_FORMAT_A8_UNORM)
        return 1;
    else
        // Do not consider extraordinary formats.
        return 0;
}
#endif
} // namespace xray::render::fg
