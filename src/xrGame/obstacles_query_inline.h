////////////////////////////////////////////////////////////////////////////
//	Module 		: obstacles_query_inline.h
//	Created 	: 10.04.2007
//  Modified 	: 10.04.2007
//	Author		: Dmitriy Iassenev
//	Description : obstacles query inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef OBSTACLES_QUERY_INLINE_H
#define OBSTACLES_QUERY_INLINE_H

IC	obstacles_query::obstacles_query							()
{
	init					();
}

IC	void obstacles_query::init									()
{
	m_crc					= 0;
}

IC	void obstacles_query::clear									()
{
	m_area.clear_not_free	();
	m_obstacles.clear		();
	init					();
}

IC	void obstacles_query::swap									(obstacles_query &object)
{
	m_area.swap				(object.m_area);
	m_obstacles.swap		(object.m_obstacles);
	std::swap				(m_crc,object.m_crc);
	std::swap				(m_actual,object.m_actual);
}

IC	void obstacles_query::copy									(const obstacles_query &object)
{
	m_area					= object.m_area;
	m_obstacles				= object.m_obstacles;
	m_crc					= object.m_crc;
	m_actual				= object.m_actual;
}

IC	void obstacles_query::add									(const CGameObject *object)
{
	VERIFY					(object);
	if (m_obstacles.find(object) != m_obstacles.end())
		return;

	m_actual				= false;
	m_obstacles.insert		(std::make_pair(object,u32(-1)));
}

IC	bool obstacles_query::refresh_objects						()
{
	m_actual				= false;
	u32						crc_before = crc();
	compute_area			();
	return					(crc_before != crc());
}

IC	bool obstacles_query::update_objects						(const Fvector &position, const float &radius)
{
	return					(objects_changed(position, radius) ? refresh_objects() : false);
}

IC	bool obstacles_query::operator==							(const obstacles_query &object) const
{
	if (crc() != object.crc())
		return				(false);

	if (!equal(area(),object.area()))
		return				(false);

//	if (!equal(obstacles(),object.obstacles()))
//		return				(false);

	return					(true);
}

IC	bool obstacles_query::operator!=							(const obstacles_query &object) const
{
	return					(!operator==(object));
}

IC	const obstacles_query::AREA &obstacles_query::area			() const
{
	return					(const_cast<obstacles_query*>(this)->area());
}

IC	obstacles_query::AREA &obstacles_query::area				()
{
	if (!actual())
		compute_area		();

	return					(m_area);
}

IC	const obstacles_query::OBSTACLES &obstacles_query::obstacles() const
{
	return					(m_obstacles);
}

IC	const u32 &obstacles_query::crc								() const
{
	return					(m_crc);
}

IC	const bool &obstacles_query::actual							() const
{
	return					(m_actual);
}

#endif // OBSTACLES_QUERY_INLINE_H