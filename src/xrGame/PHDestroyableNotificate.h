#pragma once

class CSE_Abstract;
class CPhysicsShellHolder;

class CPHDestroyableNotificate
{
public:
	virtual CPHDestroyableNotificate *		cast_phdestroyable_notificate			()						{return this;}
	virtual CPhysicsShellHolder*			PPhysicsShellHolder						()						=0;
	virtual						void		spawn_init								()						{}
								void		spawn_notificate						(CSE_Abstract*)			;
protected:
private:
};