////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_transition.hpp
//	Created 	: 20.12.2007
//	Author		: Alexander Dudin
//	Description : Transition class for smart_cover
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_TRANSITION_HPP_INCLUDED
#define SMART_COVER_TRANSITION_HPP_INCLUDED
#include "Common/Noncopyable.hpp"

namespace MonsterSpace
{
enum EBodyState : u32;
}; // namespace MonsterSpace

namespace smart_cover
{
namespace transitions
{
class animation_action;

class action final : private Noncopyable
{
public:
    typedef xr_vector<animation_action*> Animations;

private:
    shared_str m_precondition_functor;
    shared_str m_precondition_params;
    Animations m_animations;

public:
    action(luabind::object const& table);
    ~action();
    bool applicable() const;
    animation_action const& animation() const;
    animation_action const& animation(MonsterSpace::EBodyState const& target_body_state) const;
    IC Animations const& animations() const { return m_animations; };
private:
    void load_animations(luabind::object const& table);
};

} // namespace transitions
} // namespace smart_cover

#endif // SMART_COVER_TRANSITION_HPP_INCLUDED
