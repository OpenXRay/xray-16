#pragma once
#include "xrSound/Sound.h"

// fwd. decl.
class IGameObject;
template <class T> struct _vector3;
using Fvector = _vector3<float>;

namespace Feel
{
class ENGINE_API Sound
{
public:
    virtual void feel_sound_new(IGameObject* /*who*/, int /*type*/, const CSound_UserDataPtr& /*user_data*/,
        const Fvector& /*position*/, float /*power*/) {}
};
};
