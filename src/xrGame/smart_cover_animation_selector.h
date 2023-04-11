////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_selector.h
//	Created 	: 07.09.2007
//	Author		: Alexander Dudin
//	Description : Animation selector for smart covers
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ANIMATION_SELECTOR_H_INCLUDED
#define SMART_COVER_ANIMATION_SELECTOR_H_INCLUDED

#include "smart_cover_detail.h"
#include "Common/Noncopyable.hpp"
#include "Include/xrRender/KinematicsAnimated.h"
#include "smart_cover_animation_planner.h"

class CAI_Stalker;
class CPropertyStorage;
class IKinematicsAnimated;
class CScriptGameObject;
class CBlend;

namespace smart_cover
{
class action_base;
class wait_after_exit;

class animation_selector final : private Noncopyable
{
private:
    CPropertyStorage* m_storage;
    CAI_Stalker* m_object;
    animation_planner* m_planner;
    IKinematicsAnimated* m_skeleton_animated;
    shared_str m_animation;
    float m_previous_time{ flt_max };
    bool m_first_time{ true };
    bool m_callback_called{};

private:
    action_base* current_operator() const;

public:
    animation_selector(CAI_Stalker* object);
    ~animation_selector();
    void initialize();
    void execute();
    void finalize();
    MotionID select_animation(bool& animation_movement_controller);
    void on_animation_end();
    void modify_animation(CBlend* blend);
    void save(NET_Packet& packet);
    void load(IReader& packet);
    void setup(CAI_Stalker* object, CPropertyStorage* storage);
    IC CPropertyStorage* property_storage();
    IC animation_planner& planner();
};

} // namespace smart_cover

#include "smart_cover_animation_selector_inline.h"

#endif // SMART_COVER_ANIMATION_SELECTOR_H_INCLUDED
