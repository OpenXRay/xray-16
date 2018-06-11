#pragma once
#include "xrSound/sound.h"

// fwd. decl.
class IGameObject;
template <class T> struct _vector3;
using Fvector = _vector3<float>;

namespace Feel
{
class ENGINE_API Sound
{
public:
    virtual void feel_sound_new(
        IGameObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power){};
};
};
