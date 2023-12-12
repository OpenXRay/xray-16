#pragma once
#include "xrGameMtlLib/GameMtlLib.h"

class CPhysicsShellHolder;

class CPHSoundPlayer
{
    ref_sound m_sound;
    CPhysicsShellHolder* m_object;

public:
    void Play(SGameMtlPair* mtl_pair, const Fvector& pos);
    CPHSoundPlayer(CPhysicsShellHolder* m_object);
    virtual ~CPHSoundPlayer();

private:
};
