////////////////////////////////////////////////////////////////////////////
//	Module 		: vertex_manager_hash_fixed_inline.h
//	Created 	: 21.03.2002
//  Modified 	: 05.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Hash fixed vertex manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _path_id_type,\
		typename _index_type,\
		u32		 hash_size,\
		u32		 fix_size\
	>\
	template <\
		template <typename _T> class _vertex,\
		template <typename _T1, typename _T2> class _index_vertex,\
		typename _data_storage\
	>

#define CHashFixedVertexManager	CVertexManagerHashFixed<_path_id_type,_index_type,hash_size,fix_size>::CDataStorage<_vertex,_index_vertex,_data_storage>

TEMPLATE_SPECIALIZATION
IC	CHashFixedVertexManager::CDataStorage		(const u32 vertex_count) :
	inherited				(vertex_count),
	m_current_path_id		(_path_id_type(0))
{
	u32						memory_usage = 0;
	u32						byte_count;

	byte_count				= (hash_size)*sizeof(CGraphIndexVertex*);
	m_hash					= xr_alloc<CGraphIndexVertex*>(hash_size);
	ZeroMemory				(m_hash,byte_count);
	memory_usage			+= byte_count;

	byte_count				= (fix_size)*sizeof(CGraphIndexVertex);
	m_vertices				= xr_alloc<CGraphIndexVertex>(fix_size);
	ZeroMemory				(m_vertices,byte_count);
	memory_usage			+= byte_count;
}

TEMPLATE_SPECIALIZATION
CHashFixedVertexManager::~CDataStorage		()
{
	xr_free					(m_hash);
	xr_free					(m_vertices);
}

TEMPLATE_SPECIALIZATION
IC	void CHashFixedVertexManager::init		()
{
	inherited::init			();
	++m_current_path_id;
	m_vertex_count			= 0;
	if (!m_current_path_id) {
		++m_current_path_id;
		ZeroMemory			(m_hash,(hash_size)*sizeof(CGraphIndexVertex*));
		ZeroMemory			(m_vertices,(fix_size)*sizeof(CGraphIndexVertex));
	}
//	ZeroMemory			(m_hash,(hash_size)*sizeof(CGraphIndexVertex*));
//	ZeroMemory			(m_vertices,(fix_size)*sizeof(CGraphIndexVertex));
}

TEMPLATE_SPECIALIZATION
IC	void CHashFixedVertexManager::add_opened		(CGraphVertex &vertex)
{
	vertex.opened()			= 1;
}

TEMPLATE_SPECIALIZATION
IC	void CHashFixedVertexManager::add_closed		(CGraphVertex &vertex)
{
	vertex.opened()			= 0;
}

TEMPLATE_SPECIALIZATION
IC	typename CHashFixedVertexManager::_path_id_type CHashFixedVertexManager::current_path_id	() const
{
	return					(m_current_path_id);
}

TEMPLATE_SPECIALIZATION
IC	bool CHashFixedVertexManager::is_opened	(const CGraphVertex &vertex) const
{
	return					(vertex.opened());
}

TEMPLATE_SPECIALIZATION
IC	u32	 CHashFixedVertexManager::hash_index(const _index_type &vertex_id) const
{
	return					(hash_fixed_vertex_manager::to_u32(vertex_id) % hash_size);
}

TEMPLATE_SPECIALIZATION
IC	bool CHashFixedVertexManager::is_visited	(const _index_type &vertex_id) const
{
	u32						index = hash_index(vertex_id);
	CGraphIndexVertex		*vertex = m_hash[index];
	if (!vertex || (vertex->m_path_id != current_path_id()) || (vertex->m_hash != index))
		return				(false);
	for ( ; vertex; vertex = vertex->m_next)
		if (vertex->m_vertex->index() == vertex_id)
			return			(true);
	return					(false);
}

TEMPLATE_SPECIALIZATION
IC	bool CHashFixedVertexManager::is_closed	(const CGraphVertex &vertex) const
{
	return					(!is_opened(vertex));
}

TEMPLATE_SPECIALIZATION
IC	typename CHashFixedVertexManager::CGraphVertex &CHashFixedVertexManager::get_node	(const _index_type &vertex_id) const
{
	VERIFY					(is_visited(vertex_id));
	CGraphIndexVertex		*vertex = m_hash[hash_index(vertex_id)];
	for ( ; vertex; vertex = vertex->m_next)
		if (vertex->m_vertex->index() == vertex_id)
			return			(*vertex->m_vertex);
	NODEFAULT;
#ifdef DEBUG
	return					(*vertex->m_vertex);
#endif
}

TEMPLATE_SPECIALIZATION
IC	typename CHashFixedVertexManager::CGraphVertex &CHashFixedVertexManager::create_vertex	(CGraphVertex &vertex, const _index_type &vertex_id)
{
	// allocating new index node
	VERIFY							(m_vertex_count < fix_size);
	CGraphIndexVertex				*index_vertex = m_vertices + m_vertex_count++;
	// removing old links from the node
	if (index_vertex->m_prev) {
		index_vertex->m_prev->m_next		= index_vertex->m_next;
		if (index_vertex->m_next)
			index_vertex->m_next->m_prev	= index_vertex->m_prev;
	}
	else {
		if (index_vertex->m_next)
			index_vertex->m_next->m_prev	= 0;
		if (m_hash[index_vertex->m_hash] && (m_hash[index_vertex->m_hash]->m_path_id != current_path_id()))
			m_hash[index_vertex->m_hash]	= 0;
	}

	index_vertex->m_vertex			= &vertex;
	index_vertex->m_path_id			= current_path_id();
	vertex.index()					= vertex_id;
	
	u32								index = hash_index(vertex_id);
	CGraphIndexVertex				*_vertex = m_hash[index];
	if (!_vertex || (_vertex->m_path_id != current_path_id()) || (_vertex->m_hash != index))
		_vertex						= 0;

	m_hash[index]					= index_vertex;
	index_vertex->m_next			= _vertex;
	index_vertex->m_prev			= 0;
	if (_vertex)
		_vertex->m_prev				= index_vertex;
	index_vertex->m_hash			= index;
	return							(vertex);
}

#undef TEMPLATE_SPECIALIZATION
#undef CHashFixedVertexManager