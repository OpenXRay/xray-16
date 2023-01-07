#pragma once

#include "Common/GUID.hpp"
#include "xrCore/_fbox.h"

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

class NodePosition4
{
    u8 data[5];

    ICF void xz(u32 value) { CopyMemory(data, &value, 3); }
    ICF void y(u16 value) { CopyMemory(data + 3, &value, 2); }
public:
    ICF u32 xz() const { return ((*((u32*)data)) & 0x00ffffff); }
    ICF u32 x(u32 row) const { return (xz() / row); }
    ICF u32 z(u32 row) const { return (xz() % row); }
    ICF u32 y() const { return (*((u16*)(data + 3))); }
    friend class CLevelGraph;
    friend struct CNodePositionCompressor;
    friend struct CNodePositionConverter;
};

#ifdef _EDITOR
using NodePosition = NodePosition3;
#else
using NodePosition = NodePosition4;
#endif

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

struct NodeCompressed10
{
public:
    u8 data[12];

private:
    ICF void link(u8 link_index, u32 value)
    {
        value &= 0x007fffff;
        switch (link_index)
        {
        case 0:
            value |= *(u32*)data & 0xff800000;
            CopyMemory(data, &value, sizeof(u32));
            break;
        case 1:
            value <<= 7;
            value |= *(u32*)(data + 2) & 0xc000007f;
            CopyMemory(data + 2, &value, sizeof(u32));
            break;
        case 2:
            value <<= 6;
            value |= *(u32*)(data + 5) & 0xe000003f;
            CopyMemory(data + 5, &value, sizeof(u32));
            break;
        case 3:
            value <<= 5;
            value |= *(u32*)(data + 8) & 0xf000001f;
            CopyMemory(data + 8, &value, sizeof(u32));
            break;
        }
    }

    ICF void light(u8 value) { data[10] |= value << 4; }

public:
    NodeCover5 high;
    NodeCover5 low;
    u16 plane;
    NodePosition p;
    // 32 + 16 + 40 + 92 = 180 bits = 24.5 bytes => 25 bytes

    ICF u32 link(u8 index) const
    {
        switch (index)
        {
        case 0: return ((*(u32*)data) & 0x007fffff);
        case 1: return (((*(u32*)(data + 2)) >> 7) & 0x007fffff);
        case 2: return (((*(u32*)(data + 5)) >> 6) & 0x007fffff);
        case 3: return (((*(u32*)(data + 8)) >> 5) & 0x007fffff);
        default: NODEFAULT;
        }
        return 0;
    }

    friend class CLevelGraph;
    friend struct CNodeCompressed;
    friend class CNodeRenumberer;
    friend class CRenumbererConverter;
};

struct NodeCompressed7
{
    u8 data[12];
    NodeCover5 cover;
    u16 plane;
    NodePosition p;

    operator NodeCompressed10() const
    {
        NodeCompressed10 node;
        CopyMemory      (node.data, data, sizeof(data) / sizeof(u8));
        node.high   =   cover;
        node.low    =   cover;
        node.plane  =   plane;
        node.p      =   p;
        return node;
    }
};

using NodeCompressed = NodeCompressed10;
#pragma pack(pop)

constexpr cpcstr LEVEL_GRAPH_NAME = "level.ai";

const u32 XRCL_CURRENT_VERSION = 18; // input
const u32 XRCL_PRODUCTION_VERSION = 14; // output
const u32 CFORM_CURRENT_VERSION = 4;
const u32 MAX_NODE_BIT_COUNT = 23;

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

    XRAI_VERSION_ALLOWED    = XRAI_VERSION_SOC, // can be loaded by the engine
    XRAI_CURRENT_VERSION    = XRAI_VERSION_CS_COP
};

// Cross table and game spawn uses u32, but game graph header uses u8,
// Make sure to be within u8 bounds...
static_assert(XRAI_VERSION_ALLOWED  <= type_max<u8>);
static_assert(XRAI_CURRENT_VERSION  <= type_max<u8>);

#define ASSERT_XRAI_VERSION_MATCH(version, description)\
    R_ASSERT2((version) >= XRAI_VERSION_ALLOWED && (version) <= XRAI_CURRENT_VERSION, description);
