#ifndef	_LE_PHYSICS_H_
#define	_LE_PHYSICS_H_

#include "../../xrphysics/xrphysics.h"

class CObjectSpace;
class CScenePhyscs
{
	 CObjectSpace *m_object_space ;
     bool		   b_update_level_collision;
 public:
 	CScenePhyscs			() : m_object_space(0), b_update_level_collision(false)	{}
	~CScenePhyscs			() ;
 public:
    void	CreateWorld			();
    void 	DestroyWorld		();
    void	CreateShellsSelected();
    void	DestroyAll			();
    void	UseSimulatePoses	();
    void	UpdateLevelCollision(){ b_update_level_collision=true; }
    void	OnSceneModified		();
    bool	Simulating			();
private:
    bool 	CreateObjectSpace	(bool b_selected_only);
    void 	DestroyObjectSpace	();
};

  extern 	CScenePhyscs	g_scene_physics;
#endif
 