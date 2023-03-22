#pragma once

#include "Common/GUID.hpp"
#include "xrCore/_fbox.h"

constexpr cpcstr LEVEL_GRAPH_NAME = "level.ai";

enum fsL_Chunks : u32
{
    fsL_HEADER = 1, //*
    fsL_SHADERS = 2, //*
    fsL_VISUALS = 3, //*
    fsL_PORTALS = 4, //* - Portal polygons
    fsL_LIGHT_DYNAMIC = 6, //*
    fsL_GLOWS = 7, //* - All glows inside level
    fsL_SECTORS = 8, //* - All sectors on level
    fsL_VB = 9, //* - Static geometry
    fsL_IB = 10, //*
    fsL_SWIS = 11, //* - collapse info, usually for trees
};

enum fsESectorChunks : u32
{
    fsP_Portals = 1, // - portal polygons
    fsP_Root = 2, // - geometry root
};

enum fsSLS_Chunks : u32
{
    fsSLS_Description = 1, // Name of level
    fsSLS_ServerState = 2,
};

enum EBuildQuality : u16
{
    ebqDraft = 0,
    ebqHigh = 1,
    ebqCustom = 2,
};

#pragma pack(push, 8)
struct hdrLEVEL
{
    u16 XRLC_version;
    u16 XRLC_quality;
};

struct hdrCFORM
{
    u32 version;
    u32 vertcount;
    u32 facecount;
    Fbox aabb;
};

struct hdrNODES2
{
    u32 version;
    u32 count;
    float size;
    float size_y;
    Fbox aabb;
};

struct hdrNODES8
{
    u32 version;
    u32 count;
    float size;
    float size_y;
    Fbox aabb;
    xrGUID guid;
};

using hdrNODES = hdrNODES8;
#pragma pack(pop)

#pragma pack(push, 1)
// Used by SDK
struct NodePosition3
{
    s16 x;
    u16 y;
    s16 z;
};

static_assert(sizeof(NodePosition3) == 6);

class NodePosition4
{
    u8 data[5];

    ICF void xz(u32 value) { CopyMemory(data, &value, 3); }
    ICF void y(u16 value) { CopyMemory(data + 3, &value, 2); }

public:
    // xz-coordinates are packed into 3 bytes
    static constexpr u32 MAX_XZ = (1 << 24) - 1;
    // y-coordinate is packed into 2 bytes
    static constexpr u32 MAX_Y = (1 << 16) - 1;

    [[nodiscard]]
    ICF u32 xz() const { return ((*((u32*)data)) & 0x00ffffff); }

    [[nodiscard]]
    ICF u32 x(u32 row) const { return (xz() / row); }

    [[nodiscard]]
    ICF u32 z(u32 row) const { return (xz() % row); }

    [[nodiscard]]
    ICF u32 y() const { return (*((u16*)(data + 3))); }

    friend class CLevelGraph;
    friend struct CNodePositionCompressor;
    friend struct CNodePositionConverter;
};

static_assert(sizeof(NodePosition4) == 5);

class NodePosition12
{
    u32 m_xz;
    u16 m_y;

    ICF void xz(u32 value) { m_xz = value; }
    ICF void y(u16 value) { m_y = value; }

public:
    // xz-coordinates are packed into 4 bytes
    static constexpr u32 MAX_XZ = std::numeric_limits<u32>::max();
    // y-coordinate is packed into 2 bytes
    static constexpr u32 MAX_Y = std::numeric_limits<u16>::max();

    NodePosition12() = default;

    explicit NodePosition12(const NodePosition4& old)
        : m_xz(old.xz()), m_y(old.y()) {}

    NodePosition12& operator=(const NodePosition4& old)
    {
        m_xz = old.xz();
        m_y  = old.y();
        return *this;
    }

    [[nodiscard]]
    ICF u32 xz() const { return m_xz; }

    [[nodiscard]]
    ICF u32 x(u32 row) const { return (xz() / row); }

    [[nodiscard]]
    ICF u32 z(u32 row) const { return (xz() % row); }

    [[nodiscard]]
    ICF u32 y() const { return m_y; }

    friend class CLevelGraph;
    friend struct CNodePositionCompressor;
    friend struct CNodePositionConverter;
    friend class NodePosition4;
};

static_assert(sizeof(NodePosition12) == 6);

// https://github.com/OpenXRay/xray-soc-history/commit/2a3687c08f8834db1a226b60bcf7455b3cdec40a
struct NodeCover5
{
    u16 cover0 : 4;
    u16 cover1 : 4;
    u16 cover2 : 4;
    u16 cover3 : 4;

    ICF u16 cover(u8 index) const
    {
        switch (index)
        {
        case 0: return cover0;
        case 1: return cover1;
        case 2: return cover2;
        case 3: return cover3;
        default: NODEFAULT;
        }
        return u8(-1);
    }
};

static_assert(sizeof(NodeCover5) == 2);

struct NodeCompressed7
{
public:
    u8 data[12];      // 12 bytes
    NodeCover5 cover; // 2 bytes
    u16 plane;        // 2 bytes
    NodePosition4 p;  // 5 bytes
    // 12 + 2 + 2 + 5 = 21 bytes

public:
    static constexpr u32 NODE_BIT_COUNT = 23;
    static constexpr u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;

public:
    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK);
        case 1: return (((*(u32*)(data + 2)) >> 7) & LINK_MASK);
        case 2: return (((*(u32*)(data + 5)) >> 6) & LINK_MASK);
        case 3: return (((*(u32*)(data + 8)) >> 5) & LINK_MASK);
        default: NODEFAULT;
        }
        return 0;
    }
};

static_assert(sizeof(NodeCompressed7) == 21);

struct NodeCompressed10
{
public:
    NodeCompressed10() = default;

public:
    static constexpr u32 NODE_BIT_COUNT = 23;
    static constexpr u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;

    u8 data[12]; // 12 bytes

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= LINK_MASK;
        switch (link_index)
        {
        case 0:
            value |= *(u32*)data & ~(LINK_MASK << 0);
            CopyMemory(data, &value, sizeof(u32));
            break;
        case 1:
            value <<= 7;
            value |= *(u32*)(data + 2) & ~(LINK_MASK << 7);
            CopyMemory(data + 2, &value, sizeof(u32));
            break;
        case 2:
            value <<= 6;
            value |= *(u32*)(data + 5) & ~(LINK_MASK << 6);
            CopyMemory(data + 5, &value, sizeof(u32));
            break;
        case 3:
            value <<= 5;
            value |= *(u32*)(data + 8) & ~(LINK_MASK << 5);
            CopyMemory(data + 8, &value, sizeof(u32));
            break;
        }
    }

public:
    NodeCover5 high; // 2 bytes
    NodeCover5 low;  // 2 bytes
    u16 plane;       // 2 bytes
    NodePosition4 p; // 5 bytes
    // 12 + 2 + 2 + 2 + 5 = 23 bytes

    [[nodiscard]]
    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK);
        case 1: return (((*(u32*)(data + 2)) >> 7) & LINK_MASK);
        case 2: return (((*(u32*)(data + 5)) >> 6) & LINK_MASK);
        case 3: return (((*(u32*)(data + 8)) >> 5) & LINK_MASK);
        default: NODEFAULT;
        }
        return 0;
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

static_assert(sizeof(NodeCompressed10) == 23);

struct NodeCompressed11
{
public:
    static constexpr u32 NODE_BIT_COUNT = 25;
    static constexpr u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;
    static constexpr u32 LINK_MASK_0 = LINK_MASK;
    static constexpr u32 LINK_MASK_1 = LINK_MASK_0 << 1;
    static constexpr u32 LINK_MASK_2 = LINK_MASK_0 << 2;
    static constexpr u32 LINK_MASK_3 = LINK_MASK_0 << 3;

    u8 data[13];

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= LINK_MASK_0;
        switch (link_index)
        {
        case 0:
        {
            value |= (*(u32*)data) & ~LINK_MASK_0;
            CopyMemory(data, &value, sizeof(u32));
            break;
        }
        case 1:
        {
            value <<= 1;
            value |= (*(u32*)(data + 3)) & ~LINK_MASK_1;
            CopyMemory(data + 3, &value, sizeof(u32));
            break;
        }
        case 2:
        {
            value <<= 2;
            value |= (*(u32*)(data + 6)) & ~LINK_MASK_2;
            CopyMemory(data + 6, &value, sizeof(u32));
            break;
        }
        case 3:
        {
            value <<= 3;
            value |= (*(u32*)(data + 9)) & ~LINK_MASK_3;
            CopyMemory(data + 9, &value, sizeof(u32));
            break;
        }
        }
    }

public:
    NodeCover5 high;
    NodeCover5 low;
    u16 plane;
    NodePosition4 p;
    // 13 + 2 + 2 + 2 + 5 = 24 bytes

    [[nodiscard]]
    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK_0);
        case 1: return (((*(u32*)(data + 3)) >> 1) & LINK_MASK_0);
        case 2: return (((*(u32*)(data + 6)) >> 2) & LINK_MASK_0);
        case 3: return (((*(u32*)(data + 9)) >> 3) & LINK_MASK_0);
        default: NODEFAULT;
        }
        return 0;
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

static_assert(sizeof(NodeCompressed11) == 24);

struct NodeCompressed12
{
public:
    static constexpr u32 NODE_BIT_COUNT = 25;
    static constexpr u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;
    static constexpr u32 LINK_MASK_0 = LINK_MASK;
    static constexpr u32 LINK_MASK_1 = LINK_MASK_0 << 1;
    static constexpr u32 LINK_MASK_2 = LINK_MASK_0 << 2;
    static constexpr u32 LINK_MASK_3 = LINK_MASK_0 << 3;

    u8 data[13];

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= LINK_MASK_0;
        switch (link_index)
        {
        case 0:
        {
            value |= (*(u32*)data) & ~LINK_MASK_0;
            CopyMemory(data, &value, sizeof(u32));
            break;
        }
        case 1:
        {
            value <<= 1;
            value |= (*(u32*)(data + 3)) & ~LINK_MASK_1;
            CopyMemory(data + 3, &value, sizeof(u32));
            break;
        }
        case 2:
        {
            value <<= 2;
            value |= (*(u32*)(data + 6)) & ~LINK_MASK_2;
            CopyMemory(data + 6, &value, sizeof(u32));
            break;
        }
        case 3:
        {
            value <<= 3;
            value |= (*(u32*)(data + 9)) & ~LINK_MASK_3;
            CopyMemory(data + 9, &value, sizeof(u32));
            break;
        }
        }
    }

public:
    NodeCover5 high;
    NodeCover5 low;
    u16 plane;
    NodePosition12 p;
    // 13 + 2 + 2 + 2 + 6 = 25 bytes

    [[nodiscard]]
    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK_0);
        case 1: return (((*(u32*)(data + 3)) >> 1) & LINK_MASK_0);
        case 2: return (((*(u32*)(data + 6)) >> 2) & LINK_MASK_0);
        case 3: return (((*(u32*)(data + 9)) >> 3) & LINK_MASK_0);
        default: NODEFAULT;
        }
        return 0;
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

static_assert(sizeof(NodeCompressed12) == 25);

struct NodeCompressed13
{
public:
    NodeCompressed13() = default;
    NodeCompressed13& operator=(const NodeCompressed7& old)
    {
        link(0, old.link(0));
        link(1, old.link(1));
        link(2, old.link(2));
        link(3, old.link(3));
        high = old.cover;
        low = old.cover;
        plane = old.plane;
        p = old.p;
        return *this;
    }
    template <typename Node>
    NodeCompressed13& operator=(const Node& old)
    {
        link(0, old.link(0));
        link(1, old.link(1));
        link(2, old.link(2));
        link(3, old.link(3));
        high = old.high;
        low = old.low;
        plane = old.plane;
        p = old.p;
        return *this;
    }

public:
    static constexpr u32 NODE_BIT_COUNT = 26;
    static constexpr u32 LINK_MASK = (1 << NODE_BIT_COUNT) - 1;
    static constexpr u32 LINK_MASK_0 = LINK_MASK;
    static constexpr u32 LINK_MASK_1 = LINK_MASK_0 << 2;
    static constexpr u32 LINK_MASK_2 = LINK_MASK_0 << 4;
    static constexpr u32 LINK_MASK_3 = LINK_MASK_0 << 6;

    u8 data[13];

public:
    ICF void link(u8 link_index, u32 value)
    {
        value &= LINK_MASK_0;
        switch (link_index)
        {
        case 0:
        {
            value |= (*(u32*)data) & ~LINK_MASK_0;
            CopyMemory(data, &value, sizeof(u32));
            break;
        }
        case 1:
        {
            value <<= 2;
            value |= (*(u32*)(data + 3)) & ~LINK_MASK_1;
            CopyMemory(data + 3, &value, sizeof(u32));
            break;
        }
        case 2:
        {
            value <<= 4;
            value |= (*(u32*)(data + 6)) & ~LINK_MASK_2;
            CopyMemory(data + 6, &value, sizeof(u32));
            break;
        }
        case 3:
        {
            value <<= 6;
            value |= (*(u32*)(data + 9)) & ~LINK_MASK_3;
            CopyMemory(data + 9, &value, sizeof(u32));
            break;
        }
        }
    }

public:
    NodeCover5 high;
    NodeCover5 low;
    u16 plane;
    NodePosition12 p;
    // 13 + 2 + 2 + 2 + 6 = 25 bytes

    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & LINK_MASK_0);
        case 1: return (((*(u32*)(data + 3)) >> 2) & LINK_MASK_0);
        case 2: return (((*(u32*)(data + 6)) >> 4) & LINK_MASK_0);
        case 3: return (((*(u32*)(data + 9)) >> 6) & LINK_MASK_0);
        default: NODEFAULT;
        }
        return 0;
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

static_assert(sizeof(NodeCompressed13) == 25);
#pragma pack(pop)

#ifdef _EDITOR
using NodePosition   = NodePosition3;
#else
using NodePosition   = NodePosition12;
#endif
using NodeCompressed = NodeCompressed13;

const u32 XRCL_CURRENT_VERSION = 18; // input
const u32 XRCL_PRODUCTION_VERSION = 14; // output
const u32 CFORM_CURRENT_VERSION = 4;

enum xrAI_Versions : u8
{
    // Apr 4, 2005
    // https://github.com/OpenXRay/xray-soc-history/commit/2c3ae6abaf9876989f233bf20c5bc37551545968
    // Release SOC: builds 2945, 2947 and further
    XRAI_VERSION_SOC        = 8,

    // PRIQUEL: early CS builds on SOC engine, e.g. build 3120
    XRAI_VERSION_PRIQUEL    = 9,

    // Release CS/COP: build 3456 and further
    XRAI_VERSION_CS_COP     = 10,

    // Extended AI nodes to 25-bit links
    // https://github.com/abramcumner/xray15
    XRAI_VERSION_BORSHT     = 11,
    // Further improved AI nodes with 6 bytes position
    XRAI_VERSION_BORSHT_BIG = 12,
    // 26-bit links, removed rudimentary light data
    XRAI_VERSION_SKYLOADER  = 13,

    // Lowest version that can be loaded by the engine
    XRAI_VERSION_ALLOWED    = XRAI_VERSION_SOC,
    // Main version used by the game
    XRAI_CURRENT_VERSION    = XRAI_VERSION_SKYLOADER
};

// Cross table and game spawn uses u32, but game graph header uses u8,
// Make sure to be within u8 bounds...
static_assert(XRAI_VERSION_ALLOWED  <= type_max<u8>);
static_assert(XRAI_CURRENT_VERSION  <= type_max<u8>);

#define ASSERT_XRAI_VERSION_MATCH(version, description)\
    R_ASSERT2((version) >= XRAI_VERSION_ALLOWED && (version) <= XRAI_CURRENT_VERSION, description);
