////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_manager_fixed_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 01.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Fixed vertex manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _path_id_type,\
		typename _index_type,\
		u8 mask\
	>\
	template <\
		template <typename _T> class _vertex,\
		template <typename _T1, typename _T2> class _index_vertex,\
		typename _data_storage\
	>

#define CFixedVertexManager	CVertexManagerFixed<_path_id_type,_index_type,mask>::CDataStorage<_vertex,_index_vertex,_data_storage>

TEMPLATE_SPECIALIZATION
IC	CFixedVertexManager::CDataStorage		(const u32 vertex_count) :
	inherited				(vertex_count)
{
	m_current_path_id		= _path_id_type(0);
	m_max_node_count		= vertex_count;

	u32						memory_usage = 0;
	u32						byte_count;

	byte_count				= (vertex_count)*sizeof(CGraphIndexVertex);
	m_indexes				= xr_alloc<CGraphIndexVertex>(vertex_count);
	ZeroMemory				(m_indexes,byte_count);
	memory_usage			+= byte_count;
}

TEMPLATE_SPECIALIZATION
CFixedVertexManager::~CDataStorage		()
{
	xr_free					(m_indexes);
}

TEMPLATE_SPECIALIZATION
IC	void CFixedVertexManager::init		()
{
	inherited::init			();
	++m_current_path_id;
	if (!m_current_path_id) {
		ZeroMemory			(m_indexes,(m_max_node_count)*sizeof(CGraphIndexVertex));
		++m_current_path_id;
	}
}

TEMPLATE_SPECIALIZATION
IC	bool CFixedVertexManager::is_opened	(const CGraphVertex &vertex) const
{
	return					(!!vertex.opened());
}

TEMPLATE_SPECIALIZATION
IC	bool CFixedVertexManager::is_visited	(const _index_type &vertex_id) const
{
	VERIFY					(vertex_id < m_max_node_count);
	return					(m_indexes[vertex_id].m_path_id == m_current_path_id);
}

TEMPLATE_SPECIALIZATION
IC	bool CFixedVertexManager::is_closed	(const CGraphVertex &vertex) const
{
	return					(is_visited(vertex) && !is_opened(vertex));
}

TEMPLATE_SPECIALIZATION
IC	typename CFixedVertexManager::CGraphVertex &CFixedVertexManager::get_node	(const _index_type &vertex_id) const
{
	VERIFY					(vertex_id < m_max_node_count);
	VERIFY					(is_visited(vertex_id));
	return					(*m_indexes[vertex_id].m_vertex);
}

TEMPLATE_SPECIALIZATION
IC	typename CFixedVertexManager::CGraphVertex &CFixedVertexManager::create_vertex	(CGraphVertex &vertex, const _index_type &vertex_id)
{
	VERIFY							(vertex_id < m_max_node_count);
	m_indexes[vertex_id].m_vertex	= &vertex;
	m_indexes[vertex_id].m_path_id	= m_current_path_id;
	vertex._index					= vertex_id;
	return							(vertex);
}

TEMPLATE_SPECIALIZATION
IC	void CFixedVertexManager::add_opened		(CGraphVertex &vertex)
{
	vertex._opened			= 1;
}

TEMPLATE_SPECIALIZATION
IC	void CFixedVertexManager::add_closed		(CGraphVertex &vertex)
{
	vertex._opened			= 0;
}

TEMPLATE_SPECIALIZATION
IC	typename CFixedVertexManager::_path_id_type CFixedVertexManager::current_path_id	() const
{
	return					(m_current_path_id);
}

#undef TEMPLATE_SPECIALIZATION
#undef CFixedVertexManager