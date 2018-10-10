#pragma once
#ifndef pure_relcaseH
#define pure_relcaseH
#include "xrCore/xrDebug_macros.h"
#include "xrCore/fastdelegate.h" // XXX: Inluding this VERY heavy file for just xr_stdcall seems suboptimal.
#include "IGame_Level.h"

class IGameObject;

class ENGINE_API pure_relcase
{
private:
    int m_ID;

public:
    template <typename class_type>
    pure_relcase(void (xr_stdcall class_type::*function_to_bind)(IGameObject*))
    {
        R_ASSERT(g_pGameLevel);
        class_type* self = static_cast<class_type*>(this);
        g_pGameLevel->Objects.relcase_register(CObjectList::RELCASE_CALLBACK(self, function_to_bind), &m_ID);
    }
    virtual ~pure_relcase();
};

#endif // pure_relcaseH
