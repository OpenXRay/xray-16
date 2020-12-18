////////////////////////////////////////////////////////////////////////////
//	Module 		: level_graph.h
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Level graph
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/LevelStructure.hpp"
#include "xrAICore/Navigation/level_graph_space.h"
#include "xrAICore/Navigation/game_graph_space.h"
#include "xrAICore/Navigation/level_graph_manager.h"

namespace LevelGraph
{
class CHeader;
class CLevelVertex;
struct SSegment;
struct SContour;

enum ELineIntersections
{
    eLineIntersectionNone = u32(0),
    eLineIntersectionCollinear = u32(0),
    eLineIntersectionIntersect = u32(1),
    eLineIntersectionEqual = u32(2)
};
};

class CCoverPoint;

class XRAICORE_API CLevelGraph
{
private:
    friend class CRenumbererConverter;

public:
    typedef LevelGraph::CPosition CPosition;
    typedef LevelGraph::CHeader CHeader;
    typedef LevelGraph::CLevelVertex CLevelVertex;
    typedef LevelGraph::SSegment SSegment;
    typedef LevelGraph::SContour SContour;
    typedef LevelGraph::ELineIntersections ELineIntersections;

    using CLevelGraphManager = LevelGraph::CLevelGraphManager;

private:
    IReader* m_reader; // level graph virtual storage
    CHeader* m_header; // level graph header
    CLevelGraphManager* m_nodes; // contains nodes array
    xr_vector<bool> m_access_mask;
    GameGraph::_LEVEL_ID m_level_id; // unique level identifier
    u32 m_row_length;
    u32 m_column_length;
    u32 m_max_x;
    u32 m_max_z;

public:
    mutable CStatTimer NodeTime;

private:
    u32 vertex(const Fvector& position) const;
    u32 guess_vertex_id(u32 const& current_vertex_id, Fvector const& position) const;

public:
    typedef u32 const_iterator;
    typedef u32 const_spawn_iterator;
    typedef u32 const_death_iterator;
    typedef const CLevelVertex* const_vertex_iterator;

    struct vertex
    {
        static IC bool predicate(const u32& value, const CLevelVertex& vertex) { return (value < vertex.position().xz()); }
        static IC bool predicate2(const CLevelVertex& vertex, const u32& value) { return (vertex.position().xz() < value); }
    };

private:
    void Initialize(const char* filePath);

public:
    CLevelGraph();
    // for ai compiler
    CLevelGraph(const char* fileName);
    virtual ~CLevelGraph();
    IC const_vertex_iterator begin() const;
    IC const_vertex_iterator end() const;

    IC void set_mask(const xr_vector<u32>& mask);
    IC void set_mask_no_check(const xr_vector<u32>& mask);

    IC void set_mask(u32 vertex_id);
    IC void set_mask_no_check(u32 vertex_id);

    IC void clear_mask(const xr_vector<u32>& mask);
    IC void clear_mask_no_check(const xr_vector<u32>& mask);

    IC void clear_mask(u32 vertex_id);
    IC void clear_mask_no_check(u32 vertex_id);

    IC bool is_accessible(const u32 vertex_id) const;
    IC void level_id(const GameGraph::_LEVEL_ID& level_id);
    IC u32 max_x() const;
    IC u32 max_z() const;
    IC void begin(const CLevelVertex& vertex, const_iterator& begin, const_iterator& end) const;
    IC void begin(const CLevelVertex* vertex, const_iterator& begin, const_iterator& end) const;
    IC void begin(u32 vertex_id, const_iterator& begin, const_iterator& end) const;
    IC u32 value(const CLevelVertex& vertex, const_iterator i) const;
    IC u32 value(const CLevelVertex* vertex, const_iterator i) const;
    IC u32 value(const u32 vertex_id, const_iterator i) const;
    IC const CHeader& header() const;
    ICF bool valid_vertex_id(u32 vertex_id) const;
    IC const GameGraph::_LEVEL_ID& level_id() const;
    IC void unpack_xz(const CLevelGraph::CPosition& vertex_position, u32& x, u32& z) const;
    IC void unpack_xz(const CLevelGraph::CPosition& vertex_position, int& x, int& z) const;
    IC void unpack_xz(const CLevelGraph::CPosition& vertex_position, float& x, float& z) const;
    template <typename T>
    IC void unpack_xz(const CLevelGraph::CLevelVertex& vertex, T& x, T& z) const;
    template <typename T>
    IC void unpack_xz(const CLevelGraph::CLevelVertex* vertex, T& x, T& z) const;
    ICF CLevelVertex* vertex(u32 vertex_id) const;
    ICF u32 vertex(const CLevelVertex* vertex_p) const;
    ICF u32 vertex(const CLevelVertex& vertex_r) const;
    IC const Fvector vertex_position(const CLevelGraph::CPosition& source_position) const;
    IC const Fvector& vertex_position(Fvector& dest_position, const CLevelGraph::CPosition& source_position) const;
    IC const CLevelGraph::CPosition& vertex_position(
        CLevelGraph::CPosition& dest_position, const Fvector& source_position) const;
    IC const CLevelGraph::CPosition vertex_position(const Fvector& position) const;
    IC const Fvector vertex_position(u32 vertex_id) const;
    IC const Fvector vertex_position(const CLevelVertex& vertex) const;
    IC const Fvector vertex_position(const CLevelVertex* vertex) const;
    IC float vertex_plane_y(const CLevelVertex& vertex, const float X, const float Z) const;
    IC float vertex_plane_y(const CLevelVertex* vertex, const float X, const float Z) const;
    IC float vertex_plane_y(const u32 vertex_id, const float X, const float Z) const;
    IC float vertex_plane_y(const CLevelVertex& vertex) const;
    IC float vertex_plane_y(const CLevelVertex* vertex) const;
    IC float vertex_plane_y(const u32 vertex_id) const;
    IC bool inside(const CLevelVertex& vertex, const CLevelGraph::CPosition& vertex_position) const;
    IC bool inside(const CLevelVertex& vertex, const Fvector& vertex_position) const;
    IC bool inside(const CLevelVertex* vertex, const CLevelGraph::CPosition& vertex_position) const;
    IC bool inside(const CLevelVertex* vertex, const Fvector& vertex_position) const;
    IC bool inside(const u32 vertex_id, const CLevelGraph::CPosition& vertex_position) const;
    IC bool inside(const u32 vertex_id, const Fvector& position) const;
    IC bool inside(const u32 vertex_id, const Fvector2& position) const;
    IC bool inside(const CLevelVertex& vertex, const CLevelGraph::CPosition& vertex_position, const float epsilon) const;
    IC bool inside(const CLevelVertex& vertex, const Fvector& vertex_position, const float epsilon) const;
    IC bool inside(const CLevelVertex* vertex, const CLevelGraph::CPosition& vertex_position, const float epsilon) const;
    IC bool inside(const CLevelVertex* vertex, const Fvector& vertex_position, const float epsilon) const;
    IC bool inside(const u32 vertex_id, const CLevelGraph::CPosition& vertex_position, const float epsilon) const;
    IC bool inside(const u32 vertex_id, const Fvector& position, const float epsilon) const;
    IC void project_point(const Fplane& plane, Fvector& point) const;
    IC u32 row_length() const;
    float distance(const Fvector& position, const CLevelVertex* vertex) const;
    float distance(const Fvector& position, const u32 vertex_id) const;
    float distance(const u32 vertex_id, const Fvector& position) const;
    IC float distance(const Fvector& position, const Fvector& point0, const Fvector& point1) const;
    IC float distance(u32 vertex_id0, u32 vertex_id1) const;
    IC float distance(const CLevelVertex* tpNode0, u32 vertex_id1) const;
    IC float distance(u32 vertex_id0, const CLevelVertex* vertex) const;
    IC float distance(const CLevelVertex* node0, const CLevelVertex* node1) const;
    IC float distance(const u32 vertex_id, const CPosition& position) const;
    IC float distance(const CPosition& position, const u32 vertex_id) const;
    IC ELineIntersections intersect(
        float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float* x, float* y) const;
    IC ELineIntersections intersect_no_check(
        float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float* x, float* y) const;
    IC bool similar(const Fvector& point0, const Fvector& point1) const;
    IC bool inside(const Fvector& point, const SContour& contour) const;
    IC void intersect(SSegment& segment, const SContour& contour0, const SContour& contour1) const;
    IC float nearest(Fvector& destination, const Fvector& position, const Fvector& point0, const Fvector& point1) const;
    IC void contour(SContour& contour, u32 vertex_id) const;
    IC void contour(SContour& contour, const CLevelVertex* vertex) const;
    IC float nearest(Fvector& destination, const Fvector& position, const SContour& contour) const;
    IC bool intersect(
        Fvector& destination, const Fvector& v1, const Fvector& v2, const Fvector& v3, const Fvector& v4) const;
    IC float square(float a1, float b1, float alpha = PI_DIV_2) const;
    IC float compute_square(float angle, float AOV, float b0, float b1, float b2, float b3) const;
    IC float compute_high_square(float angle, float AOV, const CLevelVertex* vertex) const;
    IC float compute_low_square(float angle, float AOV, const CLevelVertex* vertex) const;
    IC float compute_high_square(float angle, float AOV, u32 dwNodeID) const;
    IC float compute_low_square(float angle, float AOV, u32 dwNodeID) const;
    IC float vertex_high_cover(const CLevelGraph::CLevelVertex* vertex) const;
    IC float vertex_low_cover(const CLevelGraph::CLevelVertex* vertex) const;
    IC float vertex_high_cover(const u32 vertex_id) const;
    IC float vertex_low_cover(const u32 vertex_id) const;
    float cover_in_direction(float angle, float b0, float b1, float b2, float b3) const;
    IC float high_cover_in_direction(float angle, const CLevelVertex* vertex) const;
    IC float low_cover_in_direction(float angle, const CLevelVertex* vertex) const;
    IC float high_cover_in_direction(float angle, u32 vertex_id) const;
    IC float low_cover_in_direction(float angle, u32 vertex_id) const;

    template <class _predicate>
    IC float vertex_high_cover_angle(u32 vertex_id, float inc_angle, _predicate compare_predicate) const;
    template <class _predicate>
    IC float vertex_low_cover_angle(u32 vertex_id, float inc_angle, _predicate compare_predicate) const;
    IC void set_invalid_vertex(u32& vertex_id, CLevelVertex** vertex = NULL) const;
    IC const u32 vertex_id(const CLevelGraph::CLevelVertex* vertex) const;
    u32 vertex_id(const Fvector& position) const;

private:
    IC u32 VertexInternal(u32 current_vertex_id, const Fvector& position) const;

public:
    u32 vertex(u32 current_vertex_id, const Fvector& position) const;
    void choose_point(const Fvector& start_point, const Fvector& finish_point, const SContour& contour, int vertex_id,
        Fvector& temp_point, int& saved_index) const;
    IC bool check_vertex_in_direction(u32 start_vertex_id, const Fvector& start_position, u32 finish_vertex_id) const;
    IC u32 check_position_in_direction(
        u32 start_vertex_id, const Fvector& start_position, const Fvector& finish_position) const;
    float check_position_in_direction(u32 start_vertex_id, const Fvector& start_position,
        const Fvector& finish_position, const float max_distance) const;
    float mark_nodes_in_direction(u32 start_vertex_id, const Fvector& start_position, const Fvector& direction,
        float distance, xr_vector<u32>& vertex_stack, xr_vector<bool>* vertex_marks) const;
    float mark_nodes_in_direction(u32 start_vertex_id, const Fvector& start_position, u32 finish_node,
        xr_vector<u32>& vertex_stack, xr_vector<bool>* vertex_marks) const;
    float mark_nodes_in_direction(u32 start_vertex_id, const Fvector& start_position, const Fvector& finish_point,
        xr_vector<u32>& vertex_stack, xr_vector<bool>* vertex_marks) const;
    float farthest_vertex_in_direction(u32 start_vertex_id, const Fvector& start_point, const Fvector& finish_point,
        u32& finish_vertex_id, xr_vector<bool>* tpaMarks, bool check_accessability = false) const;
    bool create_straight_path(u32 start_vertex_id, const Fvector& start_point, const Fvector& finish_point,
        xr_vector<Fvector>& tpaOutputPoints, xr_vector<u32>& tpaOutputNodes, bool bAddFirstPoint,
        bool bClearPath = true) const;
    bool create_straight_path(u32 start_vertex_id, const Fvector2& start_point, const Fvector2& finish_point,
        xr_vector<Fvector>& tpaOutputPoints, xr_vector<u32>& tpaOutputNodes, bool bAddFirstPoint,
        bool bClearPath = true) const;
    template <bool bAssignY, typename T>
    IC bool create_straight_path(u32 start_vertex_id, const Fvector2& start_point, const Fvector2& finish_point,
        xr_vector<T>& tpaOutputPoints, const T& example, bool bAddFirstPoint, bool bClearPath = true) const;
    template <typename T>
    IC void assign_y_values(xr_vector<T>& path);
    template <typename P>
    IC void iterate_vertices(const Fvector& min_position, const Fvector& max_position, const P& predicate) const;
    IC bool check_vertex_in_direction(u32 start_vertex_id, const Fvector2& start_position, u32 finish_vertex_id) const;
    IC u32 check_position_in_direction(
        u32 start_vertex_id, const Fvector2& start_position, const Fvector2& finish_position) const;
    bool check_vertex_in_direction_slow(
        u32 start_vertex_id, const Fvector2& start_position, u32 finish_vertex_id) const;
    u32 check_position_in_direction_slow(
        u32 start_vertex_id, const Fvector2& start_position, const Fvector2& finish_position) const;
    IC Fvector v3d(const Fvector2& vector2d) const;
    IC Fvector2 v2d(const Fvector& vector3d) const;
    IC bool valid_vertex_position(const Fvector& position) const;
    bool neighbour_in_direction(const Fvector& direction, u32 start_vertex_id) const;

    IC CLevelVertex* vertices() { return m_nodes->front(); }
};

IC bool operator<(const CLevelGraph::CLevelVertex& vertex, const u32& vertex_xz);
IC bool operator>(const CLevelGraph::CLevelVertex& vertex, const u32& vertex_xz);
IC bool operator==(const CLevelGraph::CLevelVertex& vertex, const u32& vertex_xz);
IC bool operator<(const u32& vertex_xz, const CLevelGraph::CLevelVertex& vertex);
IC bool operator>(const u32& vertex_xz, const CLevelGraph::CLevelVertex& vertex);
IC bool operator==(const u32& vertex_xz, const CLevelGraph::CLevelVertex& vertex);

#if defined(DEBUG) && !defined(AI_COMPILER)
extern BOOL g_bDebugNode;
extern u32 g_dwDebugNodeSource;
extern u32 g_dwDebugNodeDest;
#endif

#include "xrAICore/Navigation/level_graph_inline.h"
#include "xrAICore/Navigation/level_graph_vertex_inline.h"
