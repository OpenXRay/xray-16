////////////////////////////////////////////////////////////////////////////
//	Module 		: vision_client.cpp
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : vision client
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "vision_client.h"
#include "Entity.h"
#include "visual_memory_manager.h"

IC const CEntity& vision_client::object() const
{
    VERIFY(m_object);
    return (*m_object);
}

vision_client::vision_client(CEntity* object, const u32& update_interval) : Feel::Vision(object), m_object(object)
{
    VERIFY(m_object);

    m_visual = new CVisualMemoryManager(this);

    m_state = 0;

    shedule.t_min = update_interval;
    shedule.t_max = shedule.t_min;
    shedule_register();
}

vision_client::~vision_client()
{
    shedule_unregister();
    xr_delete(m_visual);
}

void vision_client::eye_pp_s01()
{
    Level().AIStats.VisQuery.Begin();

    Fvector c, k, j;
    float field_of_view, aspect_ratio, near_plane, far_plane;
    camera(c, k, j, field_of_view, aspect_ratio, near_plane, far_plane);

    Fmatrix mProject, mFull, mView;
    mView.build_camera_dir(c, k, j);
    m_position = c;
    mProject.build_projection(field_of_view, aspect_ratio, near_plane, far_plane);
    mFull.mul(mProject, mView);

    feel_vision_query(mFull, c);

    Level().AIStats.VisQuery.End();
}

void vision_client::eye_pp_s2()
{
    Level().AIStats.VisRayTests.Begin();

    u32 dwTime = Device.dwTimeGlobal;
    u32 dwDT = dwTime - m_time_stamp;
    m_time_stamp = dwTime;
    feel_vision_update(m_object, m_position, float(dwDT) / 1000.f, visual().transparency_threshold());

    Level().AIStats.VisRayTests.End();
}

float vision_client::shedule_Scale() { return (0.f); }
void vision_client::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    if (!object().g_Alive())
        return;

    switch (m_state)
    {
    case 0:
    {
        m_state = 1;
        eye_pp_s01();
        break;
    }
    case 1:
    {
        m_state = 0;
        eye_pp_s2();
        break;
    }
    default: NODEFAULT;
    }

    visual().update(float(dt) / 1000.f);
}

shared_str vision_client::shedule_Name() const
{
    string256 temp;
    xr_sprintf(temp, "vision_client[%s]", *object().cName());
    return (temp);
}

bool vision_client::shedule_Needed() { return (true); }
float vision_client::feel_vision_mtl_transp(IGameObject* O, u32 element)
{
    return (visual().feel_vision_mtl_transp(O, element));
}

void vision_client::reinit() { visual().reinit(); }
void vision_client::reload(LPCSTR section) { visual().reload(section); }
void vision_client::remove_links(IGameObject* object) { visual().remove_links(object); }
