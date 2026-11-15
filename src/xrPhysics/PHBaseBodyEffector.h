#pragma once

#include "xrPhysicsCore/IPhysicsCore.h"

class CPHBaseBodyEffector
{
protected:
    CharacterVirtualHandle m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;

public:
    void Init(CharacterVirtualHandle body) { m_char_handle = body; }
};
