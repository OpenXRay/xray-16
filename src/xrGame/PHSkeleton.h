#ifndef PH_SKELETON_H
#define PH_SKELETON_H

#include "../xrphysics/PHDefs.h"
#include "PHDestroyableNotificate.h"
class CPhysicsShellHolder;
class CSE_ALifePHSkeletonObject;
class CSE_Abstract;
class CSE_PHSkeleton;
class NET_Packet;
class CPHSkeleton :
 public CPHDestroyableNotificate
{
	bool				b_removing;
	static u32			existence_time;
	u32					m_remove_time;
	PHSHELL_PAIR_VECTOR m_unsplited_shels;

	shared_str			m_startup_anim;
	flags8				m_flags;

private:
	//Creating

	void	Init				()																				;

	void	ClearUnsplited		()																				;
	//Splitting

	void	PHSplit				()																				;

	void	SpawnCopy			()																				;
	//Autoremove
	bool	ReadyForRemove		()																				;
	void	RecursiveBonesCheck	(u16 id)																		;

protected:
	void			LoadNetState		(NET_Packet& P)															;
	void			UnsplitSingle		(CPHSkeleton* SO)														;

protected:
	virtual CPhysicsShellHolder*		PPhysicsShellHolder	()													=0;
	virtual CPHSkeleton	*PHSkeleton		()																		{return this;}
	virtual void	SpawnInitPhysics	(CSE_Abstract	*D)														=0;
	virtual void	SaveNetState		(NET_Packet& P)															;
	virtual	void	RestoreNetState		(CSE_PHSkeleton* po)											;
	
	virtual	void	InitServerObject	(CSE_Abstract	*D)														;//
	virtual	void	CopySpawnInit		()																		;
			void	RespawnInit			()																		;//net_Destroy
			bool	Spawn				(CSE_Abstract	*D)														;//net_spawn
			void	Update				(u32 dt)																;//shedule update
			void	Load				(LPCSTR section)														;//client load
public:
			void	SetAutoRemove		(u32 time=existence_time)												;
			void	SetNotNeedSave		()																		;
IC			bool	IsRemoving			(){return b_removing;}
			u32		DefaultExitenceTime ()																		{return existence_time;}
					CPHSkeleton			()																		;
	virtual			~CPHSkeleton		()																		;
};

#endif