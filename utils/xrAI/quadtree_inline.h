////////////////////////////////////////////////////////////////////////////
//	Module 		: quadtree_inline.h
//	Created 	: 23.03.2004
//  Modified 	: 23.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Quadtree class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _object_type>
#define CSQuadTree				CQuadTree<_object_type>

TEMPLATE_SPECIALIZATION
IC	CSQuadTree::CQuadTree		(const Fbox &box, float min_cell_size, u32 max_node_count, u32 max_list_item_count)
{
	m_leaf_count		= 0;
	m_radius			= _max(box.max.x - box.min.x, box.max.z - box.min.z)*.5f;
	m_center.add		(box.min,box.max);
	m_center.mul		(.5f);

	VERIFY				(!fis_zero(min_cell_size));
	VERIFY				(m_radius > min_cell_size);
	m_max_depth			= iFloor(log(2.f*m_radius/min_cell_size)/log(2.f) + .5f);

	m_nodes				= xr_new<CQuadNodeStorage>(max_node_count);
	m_list_items		= xr_new<CListItemStorage>(max_list_item_count);
	m_root				= 0;
}

TEMPLATE_SPECIALIZATION
CSQuadTree::~CQuadTree			()
{
	xr_delete			(m_nodes);
	xr_delete			(m_list_items);
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::clear	()
{
	START_PROFILE("Covers/clear")
	m_nodes->clear		();
	m_list_items->clear	();
	m_root				= 0;
	m_leaf_count		= 0;
	STOP_PROFILE
}

TEMPLATE_SPECIALIZATION
IC	size_t CSQuadTree::size	() const
{
	return				(m_leaf_count);
}

TEMPLATE_SPECIALIZATION
IC	u32	CSQuadTree::neighbour_index	(const Fvector &position, Fvector &center, float distance) const
{
	if (position.x <= center.x)
		if (position.z <= center.z) {
			center.x	-= distance;
			center.z	-= distance;
			return		(0);
		}
		else {
			center.x	-= distance;
			center.z	+= distance;
			return		(1);
		}
	else
		if (position.z <= center.z) {
			center.x	+= distance;
			center.z	-= distance;
			return		(2);
		}
		else {
			center.x	+= distance;
			center.z	+= distance;
			return		(3);
		}
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::insert		(_object_type *object)
{
	START_PROFILE("Covers/insert")
	Fvector				center = m_center;
	float				distance = m_radius;
	CQuadNode			**node = &m_root;
	for (int depth = 0; ; ++depth) {
		if (depth == m_max_depth) {
			CListItem			*list_item = m_list_items->get_object();
			list_item->m_object = object;
			list_item->m_next	= (CListItem*)((void*)(*node));
			*node				= (CQuadNode*)((void*)list_item);
			++m_leaf_count;
			return;
		}

		if (!*node)
			*node		= m_nodes->get_object();

		distance		*= .5f;
		u32				index = neighbour_index(object->position(),center,distance);
		VERIFY			(index < 4);
		
		node			= (*node)->m_neighbours + index;
	}
	STOP_PROFILE
}

TEMPLATE_SPECIALIZATION
IC	_object_type *CSQuadTree::find	(const Fvector &position)
{
	Fvector				center = m_center;
	float				distance = m_radius;
	CQuadNode			*node = m_root;
	for (int depth = 0; ; ++depth) {
		if (!node)
			return		(0);

		distance		*= .5f;
		u32				index = neighbour_index(position,center,distance);
		VERIFY			(index < 4);

		if (depth == m_max_depth) {
			CListItem	*leaf = ((CListItem*)((void*)(node)));
			for ( ; leaf; leaf = leaf->m_next)
				if (leaf->m_object->position().similar(position))
					return	(leaf->m_object);
			return		(0);
		}

		node			= node->m_neighbours[index];
	}
	NODEFAULT;
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::nearest	(const Fvector &position, float radius, xr_vector<_object_type*> &objects, bool clear) const
{
	START_PROFILE("Covers/nearest")
	if (clear)
		objects.clear	();
	nearest				(position,radius,objects,m_root,m_center,m_radius,0);
	STOP_PROFILE
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::nearest	(const Fvector &position, float radius, xr_vector<_object_type*> &objects, CQuadNode *node, Fvector center, float distance, int depth) const
{
	if (!node)
		return;

	if (depth == m_max_depth) {
		float		radius_sqr = _sqr(radius);
		CListItem	*leaf = ((CListItem*)((void*)(node)));
		for ( ; leaf; leaf = leaf->m_next)
			if (leaf->m_object->position().distance_to_xz_sqr(position) <= radius_sqr)
				objects.push_back	(leaf->m_object);
		return;
	}

	distance		*= .5f;
	Fvector			next_center = center;
	u32				index = neighbour_index(position,next_center,distance);
	VERIFY			(index < 4);
	if (_abs(position.z - center.z) < radius) {
		if (_abs(position.x - center.x) < radius) {
			if (_sqr(position.z - center.z) + _sqr(position.x - center.x) < _sqr(radius)) {
				nearest	(position,radius,objects,node->m_neighbours[0],next_center.set(center.x - distance,center.y,center.z - distance),distance,depth + 1);
				nearest	(position,radius,objects,node->m_neighbours[1],next_center.set(center.x - distance,center.y,center.z + distance),distance,depth + 1);
				nearest	(position,radius,objects,node->m_neighbours[2],next_center.set(center.x + distance,center.y,center.z - distance),distance,depth + 1);
				nearest	(position,radius,objects,node->m_neighbours[3],next_center.set(center.x + distance,center.y,center.z + distance),distance,depth + 1);
				return;
			}

			nearest		(position,radius,objects,node->m_neighbours[index],next_center,distance,depth + 1);

			if (position.z > center.z) {
				if (index & 1)
					nearest	(position,radius,objects,node->m_neighbours[index == 1 ? 0 : 2],next_center.set(center.x + (index == 1 ? -1 : 1)*distance,center.y,center.z - distance),distance,depth + 1);
			}
			else
				if (!(index & 1))
					nearest	(position,radius,objects,node->m_neighbours[!index ? 1 : 3],next_center.set(center.x + (!index ? -1 : 1)*distance,center.y,center.z + distance),distance,depth + 1);

			if (position.x > center.x) {
				if (index > 1)
					nearest	(position,radius,objects,node->m_neighbours[index == 2 ? 0 : 1],next_center.set(center.x - distance,center.y,center.z + (index == 2 ? -1 : 1)*distance),distance,depth + 1);
			}
			else
				if (index < 2)
					nearest	(position,radius,objects,node->m_neighbours[!index ? 2 : 3],next_center.set(center.x + distance,center.y,center.z + (!index ? -1 : 1)*distance),distance,depth + 1);

			return;
		}
		else {
			nearest		(position,radius,objects,node->m_neighbours[index],next_center,distance,depth + 1);
			
			if (position.z > center.z) {
				if (index & 1)
					nearest	(position,radius,objects,node->m_neighbours[index == 1 ? 0 : 2],next_center.set(center.x + (index == 1 ? -1 : 1)*distance,center.y,center.z - distance),distance,depth + 1);
			}
			else
				if (!(index & 1))
					nearest	(position,radius,objects,node->m_neighbours[!index ? 1 : 3],next_center.set(center.x + (!index ? -1 : 1)*distance,center.y,center.z + distance),distance,depth + 1);

			return;
		}
	}
	else {
		nearest			(position,radius,objects,node->m_neighbours[index],next_center,distance,depth + 1);

		if (_abs(position.x - center.x) < radius)
			if (position.x > center.x) {
				if (index > 1)
					nearest	(position,radius,objects,node->m_neighbours[index == 2 ? 0 : 1],next_center.set(center.x - distance,center.y,center.z + (index == 2 ? -1 : 1)*distance),distance,depth + 1);
			}
			else
				if (index < 2)
					nearest	(position,radius,objects,node->m_neighbours[!index ? 2 : 3],next_center.set(center.x + distance,center.y,center.z + (!index ? -1 : 1)*distance),distance,depth + 1);
	}
}

TEMPLATE_SPECIALIZATION
IC	_object_type *CSQuadTree::remove		(const _object_type *object)
{
	START_PROFILE("Covers/remove")
	_object_type	*_object = remove(object,m_root,m_center,m_radius,0);
	return			(_object);
	STOP_PROFILE
}

TEMPLATE_SPECIALIZATION
IC	_object_type *CSQuadTree::remove		(const _object_type *object, CQuadNode *&node, Fvector center, float distance, int depth)
{
	VERIFY			(node);
	if (depth == m_max_depth) {
		CListItem	*leaf = ((CListItem*)((void*)(node)));
		CListItem	*leaf_prev = 0;
		for ( ; leaf; leaf_prev = leaf, leaf = leaf->m_next)
			if (leaf->m_object == object) {
				if (!leaf_prev)
					node = 0;
				else
					leaf_prev->m_next = leaf->m_next;
				_object_type	*_object = leaf->m_object;
				m_list_items->remove(leaf);
				--m_leaf_count;
				return	(_object);
			}
		NODEFAULT;
	}

	distance		*= .5f;
	u32				index = neighbour_index(object->position(),center,distance);
	VERIFY			(index < 4);
	_object_type	*_object = remove(object,node->m_neighbours[index],center,distance,depth + 1);
	if (node->m_neighbours[index] || node->m_neighbours[0] || node->m_neighbours[1] || node->m_neighbours[2] || node->m_neighbours[3])
		return		(_object);
	m_nodes->remove	(node);
	return			(_object);
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::all		(xr_vector<_object_type*> &objects, CQuadNode *node, int depth) const
{
	if (!node)
		return;

	if (depth == m_max_depth) {
		CListItem				*leaf = ((CListItem*)((void*)(node)));
		for ( ; leaf; leaf = leaf->m_next)
			objects.push_back	(leaf->m_object);
		return;
	}

	all							(objects,node->m_neighbours[0],depth + 1);
	all							(objects,node->m_neighbours[1],depth + 1);
	all							(objects,node->m_neighbours[2],depth + 1);
	all							(objects,node->m_neighbours[3],depth + 1);
}

TEMPLATE_SPECIALIZATION
IC	void CSQuadTree::all		(xr_vector<_object_type*> &objects, bool clear = true) const
{
	if (clear)
		objects.clear			();
	all							(objects,m_root,0);
}

#undef TEMPLATE_SPECIALIZATION
#undef CSQuadTree