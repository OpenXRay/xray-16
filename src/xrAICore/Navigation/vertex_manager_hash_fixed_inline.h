////////////////////////////////////////////////////////////////////////////
//  Module      : vertex_manager_hash_fixed_inline.h
//  Created     : 21.03.2002
//  Modified    : 05.03.2004
//  Author      : Dmitriy Iassenev
//  Description : Hash fixed vertex manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION                                             \
    template <typename TPathId, typename TIndex, u32 HashSize, u32 FixSize> \
    template <typename TPathBuilder, typename TVertexAllocator, typename TCompoundVertex>

#define CHashFixedVertexManager                                                                               \
    CVertexManagerHashFixed<TPathId, TIndex, HashSize, FixSize>::CDataStorage<TPathBuilder, TVertexAllocator, \
        TCompoundVertex>

#define CHashFixedVertexManagerT                                                                              \
    typename CVertexManagerHashFixed<TPathId, TIndex, HashSize, FixSize>::template CDataStorage<TPathBuilder, \
        TVertexAllocator, TCompoundVertex>

TEMPLATE_SPECIALIZATION
inline CHashFixedVertexManager::CDataStorage(const u32 vertex_count)
    : CDataStorageBase(vertex_count), CDataStorageAllocator(), m_current_path_id(PathId(0))
{
    m_hash = xr_alloc<IndexVertex*>(HashSize);
    ZeroMemory(m_hash, HashSize * sizeof(IndexVertex*));
    m_vertices = xr_alloc<IndexVertex>(FixSize);
    ZeroMemory(m_vertices, FixSize * sizeof(IndexVertex));
}

TEMPLATE_SPECIALIZATION
CHashFixedVertexManager::~CDataStorage()
{
    xr_free(m_hash);
    xr_free(m_vertices);
}

TEMPLATE_SPECIALIZATION
inline void CHashFixedVertexManager::init()
{
    CDataStorageBase::init();
    CDataStorageAllocator::init();
    m_current_path_id++;
    m_vertex_count = 0;
    if (!m_current_path_id)
    {
        m_current_path_id++;
        ZeroMemory(m_hash, HashSize * sizeof(IndexVertex*));
        ZeroMemory(m_vertices, FixSize * sizeof(IndexVertex));
    }
}

TEMPLATE_SPECIALIZATION
inline void CHashFixedVertexManager::add_opened(Vertex& vertex) { vertex.opened() = 1; }
TEMPLATE_SPECIALIZATION
inline void CHashFixedVertexManager::add_closed(Vertex& vertex) { vertex.opened() = 0; }
TEMPLATE_SPECIALIZATION
inline CHashFixedVertexManagerT::PathId CHashFixedVertexManager::current_path_id() const
{
    return m_current_path_id;
}

TEMPLATE_SPECIALIZATION
inline bool CHashFixedVertexManager::is_opened(const Vertex& vertex) const { return vertex.opened(); }
TEMPLATE_SPECIALIZATION
inline u32 CHashFixedVertexManager::hash_index(const Index& vertex_id) const
{
#ifdef LINUX // FIXME!!
    return 0;
#else
    return hash_fixed_vertex_manager::to_u32(vertex_id) % HashSize;
#endif
}

TEMPLATE_SPECIALIZATION
inline bool CHashFixedVertexManager::is_visited(const Index& vertex_id) const
{
    u32 index = hash_index(vertex_id);
    IndexVertex* vertex = m_hash[index];
    if (!vertex || vertex->m_path_id != current_path_id() || vertex->m_hash != index)
        return false;
    for (; vertex; vertex = vertex->m_next)
    {
        if (vertex->m_vertex->index() == vertex_id)
            return true;
    }
    return false;
}

TEMPLATE_SPECIALIZATION
inline bool CHashFixedVertexManager::is_closed(const Vertex& vertex) const { return !is_opened(vertex); }
TEMPLATE_SPECIALIZATION
inline CHashFixedVertexManagerT::Vertex& CHashFixedVertexManager::get_node(const Index& vertex_id) const
{
    VERIFY(is_visited(vertex_id));
    IndexVertex* vertex = m_hash[hash_index(vertex_id)];
    for (; vertex; vertex = vertex->m_next)
    {
        if (vertex->m_vertex->index() == vertex_id)
            return *vertex->m_vertex;
    }
    NODEFAULT;
    return *vertex->m_vertex;
}

TEMPLATE_SPECIALIZATION
inline CHashFixedVertexManagerT::Vertex& CHashFixedVertexManager::create_vertex(
    Vertex& vertex, const Index& vertex_id)
{
    // allocating new index node
    VERIFY(m_vertex_count < FixSize);
    IndexVertex* index_vertex = m_vertices + m_vertex_count++;
    // removing old links from the node
    if (index_vertex->m_prev)
    {
        index_vertex->m_prev->m_next = index_vertex->m_next;
        if (index_vertex->m_next)
            index_vertex->m_next->m_prev = index_vertex->m_prev;
    }
    else
    {
        if (index_vertex->m_next)
            index_vertex->m_next->m_prev = nullptr;
        if (m_hash[index_vertex->m_hash] && m_hash[index_vertex->m_hash]->m_path_id != current_path_id())
            m_hash[index_vertex->m_hash] = nullptr;
    }
    index_vertex->m_vertex = &vertex;
    index_vertex->m_path_id = current_path_id();
    vertex.index() = vertex_id;
    u32 index = hash_index(vertex_id);
    IndexVertex* _vertex = m_hash[index];
    if (!_vertex || _vertex->m_path_id != current_path_id() || _vertex->m_hash != index)
        _vertex = nullptr;
    m_hash[index] = index_vertex;
    index_vertex->m_next = _vertex;
    index_vertex->m_prev = nullptr;
    if (_vertex)
        _vertex->m_prev = index_vertex;
    index_vertex->m_hash = index;
    return vertex;
}

#undef TEMPLATE_SPECIALIZATION
#undef CHashFixedVertexManager
