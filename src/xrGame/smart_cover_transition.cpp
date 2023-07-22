////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_transition.cpp
//	Created 	: 20.12.2007
//	Author		: Alexander Dudin
//	Description : Transition class for smart_cover
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_transition.hpp"
#include "smart_cover_transition_animation.hpp"
#include "smart_cover_detail.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "Common/object_broker.h"

namespace smart_cover::transitions
{
using smart_cover::detail::parse_table;
using smart_cover::detail::parse_string;
using smart_cover::detail::parse_fvector;
using smart_cover::detail::parse_int;

action::action(luabind::object const& table)
{
    VERIFY(luabind::type(table) == LUA_TTABLE);

    m_precondition_functor = parse_string(table, "precondition_functor");
    m_precondition_params = parse_string(table, "precondition_params");

    luabind::object anim_table;
    parse_table(table, "actions", anim_table);
    load_animations(anim_table);
}

action::~action() { delete_data(m_animations); }
bool action::applicable() const
{
    luabind::functor<bool> functor;

    R_ASSERT2(GEnv.ScriptEngine->functor(m_precondition_functor.c_str(), functor),
        make_string("failed to get [%s]", m_precondition_functor.c_str()));

    return (functor(m_precondition_params.c_str()));
}

void action::load_animations(luabind::object const& table)
{
    luabind::iterator it(table), end;
    const size_t count = luabind_it_distance(it, end);
    m_animations.reserve(count);
    while (it != end)
    {
        auto tmp = *it;
        const Fvector pos = parse_fvector(tmp, "position");
        const shared_str anim_id = parse_string(tmp, "animation");
        const auto body_state = (MonsterSpace::EBodyState)parse_int(tmp, "body_state");
        const auto movement_type = (MonsterSpace::EMovementType)parse_int(tmp, "movement_type");
        auto* animation = xr_new<animation_action>(pos, anim_id, body_state, movement_type);
        m_animations.push_back(animation);
        ++it;
    }
}

class body_state_predicate
{
    MonsterSpace::EBodyState m_body_state;

public:
    IC body_state_predicate(MonsterSpace::EBodyState const& body_state) : m_body_state(body_state) {}
    IC bool operator()(animation_action* animation_action) const
    {
        VERIFY(animation_action);

        // if (!animation_action->has_animation())
        //	return				(false);

        return (m_body_state == animation_action->body_state());
    }
};

animation_action const& action::animation(MonsterSpace::EBodyState const& target_body_state) const
{
    Animations::const_iterator found =
        std::find_if(m_animations.begin(), m_animations.end(), body_state_predicate(target_body_state));

    if (found == m_animations.end())
    {
#ifndef MASTER_GOLD
        Msg("! There is no animation which can transfer bot to body_state [%i], selecting random transition",
            target_body_state);
#endif // #ifndef MASTER_GOLD
        return (animation());
    }

#if 0 // for testing
	VERIFY2						(
		found != m_animations.end(),
		make_string("There is no animation which can transfer bot to body_state [%i]", target_body_state)
	);
#endif

    return (**found);
}

animation_action const& action::animation() const { return (*m_animations[Random.randI(m_animations.size())]); }
} // namespace smart_cover::transitions
