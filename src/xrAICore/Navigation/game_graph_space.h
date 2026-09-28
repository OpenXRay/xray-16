////////////////////////////////////////////////////////////////////////////
//	Module 		: game_graph_space.h
//	Created 	: 18.02.2003
//  Modified 	: 11.12.2004
//	Author		: Dmitriy Iassenev
//	Description : Game graph namespace
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCore/Containers/AssociativeVector.hpp"
#include "xrCore/FixedVector.h"
#include "Common/GUID.hpp"

namespace GameGraph
{
typedef u16 _GRAPH_ID;
typedef u8 _LEVEL_ID;
typedef u8 _LOCATION_ID;

enum
{
    LOCATION_TYPE_COUNT = 4,
    LOCATION_COUNT = (u32(1) << (8 * sizeof(_LOCATION_ID))),
};

#ifdef AI_COMPILER
struct
#else
class
#endif
    SLevel
{
    shared_str m_name;
    Fvector m_offset;
    _LEVEL_ID m_id;
    shared_str m_section;
    xrGUID m_guid;

public:
    IC const shared_str& name() const { return (m_name); }
    IC const Fvector& offset() const { return (m_offset); }
    IC const _LEVEL_ID& id() const { return (m_id); }
    IC const shared_str& section() const { return (m_section); }
    IC const xrGUID& guid() const { return (m_guid); }
    IC void load(IReader* reader);
    IC void save(IWriter* writer);

    friend class CGameGraph;
};

typedef AssociativeVector<_LEVEL_ID, SLevel> LEVEL_MAP;

#pragma pack(push, 1)
#ifdef AI_COMPILER
struct
#else
class
#endif
    CEdge
{
    _GRAPH_ID m_vertex_id;
    float m_path_distance;

public:
    IC _GRAPH_ID vertex_id() const;
    IC float distance() const;
};
class CGameVertex
{
#ifdef AI_COMPILER
public:
#else
private:
#endif
    Fvector tLocalPoint;
    Fvector tGlobalPoint;
    u32 tLevelID : 8;
    u32 tNodeID : 24;
    u8 tVertexTypes[LOCATION_TYPE_COUNT];
    u32 dwEdgeOffset;
    u32 dwPointOffset;
    u8 tNeighbourCount;
    u8 tDeathPointCount;

public:
    IC Fvector level_point() const;
    IC Fvector game_point() const;
    IC _LEVEL_ID level_id() const;
    IC u32 level_vertex_id() const;
    IC const u8* vertex_type() const;
    IC const u8& edge_count() const;
    IC u32 edge_offset() const;
    IC const u8& death_point_count() const;
    IC u32 death_point_offset() const;
    friend class CGameGraph;
};

// CEdge and CGameVertex are views of the original packed game.graph data.
// Keep their exact disk layout, but never return references to multi-byte
// fields: mapped chunk starts and packed members are not guaranteed to be
// naturally aligned on ARM.
static_assert(sizeof(CEdge) == 6);
static_assert(sizeof(CGameVertex) == 42);
#pragma pack(pop)

class CHeader
{
#ifdef AI_COMPILER
public:
#else
private:
#endif
    u8 m_version;
    _GRAPH_ID m_vertex_count;
    u32 m_edge_count;
    u32 m_death_point_count;
    xrGUID m_guid;
    LEVEL_MAP m_levels;

public:
    IC u8 version() const;
    IC _LEVEL_ID level_count() const;
    IC _GRAPH_ID vertex_count() const;
    IC u32 edge_count() const;
    IC u32 death_point_count() const;
    IC const xrGUID& guid() const;
    IC const LEVEL_MAP& levels() const;
    IC bool level_exist(const _LEVEL_ID& id) const;
    IC bool level_exist(pcstr level_name) const;
    IC const SLevel& level(const _LEVEL_ID& id) const;
    IC const SLevel& level(LPCSTR level_name) const;
    IC const SLevel* level(LPCSTR level_name, bool) const;
    IC void load(IReader* reader);
    IC void save(IWriter* reader);
    friend class CGameGraph;
};

#pragma pack(push, 1)
#ifdef AI_COMPILER
struct
#else
class
#endif
    CLevelPoint
{
    Fvector tPoint;
    u32 tNodeID;
    float fDistance;

public:
    IC Fvector level_point() const;
    IC u32 level_vertex_id() const;
    IC float distance() const;
};

// Spawn points are serialized immediately after the packed graph records and
// therefore inherit their byte alignment. Their on-disk size is unchanged.
static_assert(sizeof(CLevelPoint) == 20);
#pragma pack(pop)

struct STerrainPlace
{
    svector<_LOCATION_ID, LOCATION_TYPE_COUNT> tMask;
};

using TERRAIN_VECTOR = xr_vector<STerrainPlace>;
}
