#include "stdafx.h"

#include "PHSoundPlayer.h"
#include "PhysicsShellHolder.h"
CPHSoundPlayer::CPHSoundPlayer(CPhysicsShellHolder* obj)
{

	m_object=obj;
}

CPHSoundPlayer::~CPHSoundPlayer()
{
	m_sound.stop();
	m_object=NULL;
}

void CPHSoundPlayer::Play(SGameMtlPair* mtl_pair,const Fvector& pos)
{

	if(!m_sound._feedback())
	{
		Fvector vel;m_object->PHGetLinearVell(vel);
		if(vel.square_magnitude()>0.01f)
		{
			CLONE_MTL_SOUND(m_sound, mtl_pair, CollideSounds);
			m_sound.play_at_pos(smart_cast<CPhysicsShellHolder*>(m_object),pos);
		}
	}
}


