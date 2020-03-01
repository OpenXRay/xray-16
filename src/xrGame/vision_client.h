////////////////////////////////////////////////////////////////////////////
//	Module 		: vision_client.h
//	Created 	: 11.06.2007
//  Modified 	: 11.06.2007
//	Author		: Dmitriy Iassenev
//	Description : vision client
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef VISION_CLIENT_H
#define VISION_CLIENT_H

#include "xrEngine/ISheduled.h"
#include "xrEngine/Feel_Vision.h"

class IGameObject;
class CEntity;
class CVisualMemoryManager;

class vision_client : public ScheduledBase, public Feel::Vision
{
private:
    typedef ScheduledBase inherited;

private:
    CEntity* m_object;
    CVisualMemoryManager* m_visual;

private:
    u32 m_state;
    u32 m_time_stamp;
    Fvector m_position;

private:
    IC const CEntity& object() const;

private:
    void eye_pp_s01();
    void eye_pp_s2();

public:
    vision_client(CEntity* object, const u32& update_interval);
    virtual ~vision_client();

public:
    virtual float shedule_Scale();
    virtual void shedule_Update(u32 dt);
    virtual shared_str shedule_Name() const;
    virtual bool shedule_Needed();

public:
    virtual float feel_vision_mtl_transp(IGameObject* object, u32 element);

public:
    virtual bool feel_vision_isRelevant(IGameObject* object) = 0;
    virtual void camera(Fvector& position, Fvector& direction, Fvector& normal, float& field_of_view,
        float& aspect_ratio, float& near_plane, float& far_plane) = 0;

public:
    virtual void reinit();
    virtual void reload(LPCSTR section);
    void remove_links(IGameObject* object);

public:
    IC CVisualMemoryManager& visual() const;
};

#include "vision_client_inline.h"

#endif // VISION_CLIENT_H
