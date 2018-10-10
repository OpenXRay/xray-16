#include "StdAfx.h"

#include "PHSoundPlayer.h"
#include "PhysicsShellHolder.h"
CPHSoundPlayer::CPHSoundPlayer(CPhysicsShellHolder* obj) { m_object = obj; }
CPHSoundPlayer::~CPHSoundPlayer()
{
    m_sound.stop();
    m_object = NULL;
}

void CPHSoundPlayer::Play(SGameMtlPair* mtl_pair, const Fvector& pos)
{
    if (!m_sound._feedback())
    {
        Fvector vel;
        m_object->PHGetLinearVell(vel);
        if (vel.square_magnitude() > 0.01f)
        {
            VERIFY2(!mtl_pair->CollideSounds.empty(), mtl_pair->dbg_Name());
            ref_sound& randSound = mtl_pair->CollideSounds[Random.randI(mtl_pair->CollideSounds.size())];
            m_sound.clone(randSound, st_Effect, sg_SourceType);
            m_sound.play_at_pos(smart_cast<CPhysicsShellHolder*>(m_object), pos);
        }
    }
}
