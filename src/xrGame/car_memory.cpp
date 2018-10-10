////////////////////////////////////////////////////////////////////////////
//	Module 		: car_memory.cpp
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : car memory
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "car_memory.h"
#include "Car.h"

car_memory::car_memory(CCar* object) : inherited(object, 100), m_object(object)
{
    VERIFY(m_object);
    m_view_position.set(0, 0, 0);
    m_view_direction.set(0, 0, 1);
    m_view_normal.set(0, 1, 0);
    m_far_plane = 1.0f;
}

void car_memory::reload(LPCSTR section)
{
    inherited::reload(section);
    m_fov_deg = pSettings->r_float(section, "view_fov_deg");
    m_aspect = pSettings->r_float(section, "view_aspect");
    m_far_plane = pSettings->r_float(section, "view_far_plane");
}
#include "Actor.h"
bool car_memory::feel_vision_isRelevant(IGameObject* object)
{
    return (NULL != smart_cast<CActor*>(object));
    //.	return			(false);
}

void car_memory::camera(Fvector& position, Fvector& direction, Fvector& normal, float& field_of_view,
    float& aspect_ratio, float& near_plane, float& far_plane)
{
    position = m_view_position;
    direction = m_view_direction;
    normal = m_view_normal;

    field_of_view = deg2rad(m_fov_deg);
    aspect_ratio = m_aspect;
    near_plane = .1f;
    far_plane = m_far_plane;
}

void car_memory::set_camera(const Fvector& position, const Fvector& direction, const Fvector& normal)
{
    m_view_position = position;
    m_view_direction = direction;
    m_view_normal = normal;
}
