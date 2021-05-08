////////////////////////////////////////////////////////////////////////////
//	Created		: 29.07.2009
//	Author		: Armen Abroyan
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef STATS_MANAGER_H_INCLUDED
#define STATS_MANAGER_H_INCLUDED

enum enum_stats_buffer_type
{
    enum_stats_buffer_type_vertex,
    enum_stats_buffer_type_index,
    enum_stats_buffer_type_rtarget,
    enum_stats_buffer_type_COUNT
};

class stats_manager
{
public:
    ~stats_manager();
    void increment_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location);
    void decrement_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location);

    void increment_stats_rtarget(ID3DTexture2D* buff);
    void increment_stats_vb(ID3DVertexBuffer* buff);
    void increment_stats_ib(ID3DIndexBuffer* buff);

    void decrement_stats_rtarget(ID3DTexture2D* buff);
    void decrement_stats_vb(ID3DVertexBuffer* buff);
    void decrement_stats_ib(ID3DIndexBuffer* buff);

    u32 memory_usage_summary[enum_stats_buffer_type_COUNT][4];

private:
    void increment_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location, void* buff_ptr);
    void decrement_stats(u32 size, enum_stats_buffer_type type, _D3DPOOL location, void* buff_ptr);

#ifdef DEBUG
    struct stats_item
    {
        void* buff_ptr;
        u32 size;
        enum_stats_buffer_type type;
        _D3DPOOL location;
    }; // stats_item

    xr_vector<stats_item> m_buffers_list;
#endif
}; // class stats_manager

u32 get_format_pixel_size(D3DFORMAT format);

#ifndef USE_DX9
u32 get_format_pixel_size(DXGI_FORMAT format);
#endif

#endif // #ifndef STATS_MANAGER_H_INCLUDED
