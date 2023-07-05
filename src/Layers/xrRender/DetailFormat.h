#pragma once

#pragma pack(push, 1)

#define DETAIL_VERSION 3
#define DETAIL_SLOT_SIZE 2.f
#define DETAIL_SLOT_SIZE_2 DETAIL_SLOT_SIZE * 0.5f

// int s_x = iFloor(EYE.x/slot_size+.5f)+offs_x; // [0...size_x)
// int s_z = iFloor(EYE.z/slot_size+.5f)+offs_z; // [0...size_z)

/*
0 - Header(version,obj_count(max255),size_x,size_z,min_x,min_z)
1 - Objects
    0
    1
    2
    ..
    obj_count-1
2 - slots

    CMemoryWriter F;
    m_Header.object_count=m_Objects.size();
    // header
    F.w_chunk       (DETMGR_CHUNK_HEADER,&m_Header,sizeof(DetailHeader));
    // objects
    F.open_chunk        (DETMGR_CHUNK_OBJECTS);
    for (DOIt it=m_Objects.begin(); it!=m_Objects.end(); it++){
        F.open_chunk    (it-m_Objects.begin());
        (*it)->Export   (F);
        F.close_chunk   ();
    }
    F.close_chunk       ();
    // slots
    F.open_chunk        (DETMGR_CHUNK_SLOTS);
    F.write             (m_Slots.begin(),m_Slots.size()*sizeof(DetailSlot));
    F.close_chunk       ();

    F.SaveTo            (fn,0);
*/
/*
// detail object
    char*           shader;
    char*           texture;

    u32             flag;
    float           min_scale;
    float           max_scale;

    u32             vert_count;
    u32             index_count;

    fvfVertexIn*    vertices;
    u16*            indices;
*/

class EDetailManager;
#define DO_NO_WAVING 0x0001

class DetailHeader
{
private:
    u32 _version;
    u32 obj_count;
    int offs_x, offs_z;
    u32 size_x, size_z;

public:
    DetailHeader()
    {
        _version = u32(-1);
        obj_count = u32(-1);
        offs_x = offs_z = u32(-1);
        size_x = size_z = u32(-1);
    }

    u32 version() const { return _version; }
    float fromSlotX(int x) const { return (x - offs_x) * DETAIL_SLOT_SIZE + DETAIL_SLOT_SIZE_2; }
    float fromSlotZ(int z) const { return (z - offs_z) * DETAIL_SLOT_SIZE + DETAIL_SLOT_SIZE_2; }

    void GetSlotRect(Frect& rect, int sx, int sz) const
    {
        float x = fromSlotX(sx);
        float z = fromSlotZ(sz);
        rect.x1 = x - DETAIL_SLOT_SIZE_2 + EPS_L;
        rect.y1 = z - DETAIL_SLOT_SIZE_2 + EPS_L;
        rect.x2 = x + DETAIL_SLOT_SIZE_2 - EPS_L;
        rect.y2 = z + DETAIL_SLOT_SIZE_2 - EPS_L;
    }

    u32 object_count() const { return obj_count; }
    u32 x_size() const { return size_x; }
    u32 z_size() const { return size_z; }
    u32 x_offs() const { return offs_x; }
    u32 z_offs() const { return offs_z; }

    u32 slot_index(int _x, int _z) const
    {
        u32 ret = _z * size_x + _x;
        int xx, zz;
        slot_x_z(ret, xx, zz);
        R_ASSERT(zz == _z);
        R_ASSERT(xx == _x);
        return ret;
    }

    void slot_x_z(u32 idx, int& _x, int& _z) const
    {
        VERIFY(idx < slot_count());
        _z = idx / size_x;
        _x = idx % size_x;
        VERIFY(u32(_z) < z_size());
        VERIFY(u32(_x) < x_size());
    }

    u32 slot_count() const { return size_x * size_z; }
    float slot_min_x(int _x) const { return (int(_x) - int(offs_x)) * DETAIL_SLOT_SIZE; }
    float slot_min_z(int _z) const { return (int(_z) - int(offs_z)) * DETAIL_SLOT_SIZE; }

    friend class EDetailManager;
};

struct DetailPalette
{
    u16 a0 : 4;
    u16 a1 : 4;
    u16 a2 : 4;
    u16 a3 : 4;
};
struct DetailSlot // was(4+4+3*4+2 = 22b), now(8+2*4=16b)
{
    u32 y_base : 12; // 11 // 1 unit = 20 cm, low = -200m, high = 4096*20cm - 200 = 619.2m
    u32 y_height : 8; // 20 // 1 unit = 10 cm, low = 0, high = 256*10 ~= 25.6m
    u32 id0 : 6; // 26 // 0x3F(63) = empty
    u32 id1 : 6; // 32 // 0x3F(63) = empty
    u32 id2 : 6; // 38 // 0x3F(63) = empty
    u32 id3 : 6; // 42 // 0x3F(63) = empty
    u32 c_dir : 4; // 48 // 0..1 q
    u32 c_hemi : 4; // 52 // 0..1 q
    u32 c_r : 4; // 56 // rgb = 4.4.4
    u32 c_g : 4; // 60 // rgb = 4.4.4
    u32 c_b : 4; // 64 // rgb = 4.4.4
    DetailPalette palette[4];

public:
    enum
    {
        ID_Empty = 0x3f
    };

    void w_y(float base, float height)
    {
        s32 _base = iFloor((base + 200) / .2f);
        clamp(_base, 0, 4095);
        y_base = _base;
        f32 _error = base - r_ybase();
        s32 _height = iCeil((height + _error) / .1f);
        clamp(_height, 0, 255);
        y_height = _height;
    }

    float r_ybase() const { return float(y_base) * .2f - 200.f; }
    float r_yheight() const { return float(y_height) * .1f; }
    u32 w_qclr(float v, u32 range) const
    {
        s32 _v = iFloor(v * float(range));
        clamp(_v, 0, s32(range));
        return _v;
    }

    float r_qclr(u32 v, u32 range) const { return float(v) / float(range); }
    void color_editor()
    {
        c_dir = w_qclr(0.5f, 15);
        c_hemi = w_qclr(0.5f, 15);
        c_r = w_qclr(0.f, 15);
        c_g = w_qclr(0.f, 15);
        c_b = w_qclr(0.f, 15);
    }

    u8 r_id(u32 idx)
    {
        switch (idx)
        {
        case 0: return (u8)id0;
        case 1: return (u8)id1;
        case 2: return (u8)id2;
        case 3: return (u8)id3;
        default: NODEFAULT;
        }
#ifdef DEBUG
        return 0;
#endif
    }

    void w_id(u32 idx, u8 val)
    {
        switch (idx)
        {
        case 0: id0 = val; break;
        case 1: id1 = val; break;
        case 2: id2 = val; break;
        case 3: id3 = val; break;
        default: NODEFAULT;
        }
    }
};

IC bool is_empty(const DetailPalette& pallete) { return !pallete.a0 && !pallete.a1 && !pallete.a2 && !pallete.a3; }
IC bool is_empty(const DetailSlot& DS)
{
    return DS.id0 == DetailSlot::ID_Empty && DS.id1 == DetailSlot::ID_Empty && DS.id2 == DetailSlot::ID_Empty &&
        DS.id3 == DetailSlot::ID_Empty;
}

IC void process_pallete(DetailSlot& DS)
{
    if (is_empty(DS.palette[0]))
        DS.id0 = DetailSlot::ID_Empty;
    if (is_empty(DS.palette[1]))
        DS.id1 = DetailSlot::ID_Empty;
    if (is_empty(DS.palette[2]))
        DS.id2 = DetailSlot::ID_Empty;
    if (is_empty(DS.palette[3]))
        DS.id3 = DetailSlot::ID_Empty;
}

IC Fvector& get_slot_diameter(Fvector& diameter, const DetailSlot& DS)
{
    diameter.set(DETAIL_SLOT_SIZE, DS.r_yheight(), DETAIL_SLOT_SIZE);
    return diameter;
}

#pragma pack(pop)
