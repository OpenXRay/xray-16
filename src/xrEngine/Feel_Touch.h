#pragma once

#include "pure_relcase.h"
#include "xrCommon/xr_vector.h"

// fwd. decl.
class IGameObject;
template <class T> struct _vector3;
using Fvector = _vector3<float>;

namespace Feel
{
class ENGINE_API Touch : private pure_relcase
{
    friend class pure_relcase;

public:
    struct DenyTouch
    {
        IGameObject* O;
        /*DWORD*/ unsigned long Expire;
    };

protected:
    xr_vector<DenyTouch> feel_touch_disable;

public:
    xr_vector<IGameObject*> feel_touch;
    xr_vector<IGameObject*> q_nearest;

public:
    void __stdcall feel_touch_relcase(IGameObject* O);

public:
    Touch();
    virtual ~Touch();

    virtual bool feel_touch_contact(IGameObject* O);
    virtual void feel_touch_update(Fvector& P, float R);
    virtual void feel_touch_deny(IGameObject* O, u32 T);
    virtual void feel_touch_new(IGameObject* O){};
    virtual void feel_touch_delete(IGameObject* O){};
};
};
