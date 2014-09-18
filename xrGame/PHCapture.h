#pragma	 once
#ifndef PH_CAPTURE_H
#define PH_CAPTURE_H

#include "phobject.h"
#include "gameobject.h"
#include "physicsshellholder.h"

class	CPhysicShellHolder;
class	CPHCharacter;
class	CPhysicsElement;
struct	CPHCaptureBoneCallback;
class CPHCapture : public CPHUpdateObject
{
public:
					CPHCapture	(CPHCharacter     *a_character,CPhysicsShellHolder	  *a_taget_object, CPHCaptureBoneCallback* cb /*=0*/);
					CPHCapture	(CPHCharacter     *a_character,CPhysicsShellHolder	  *a_taget_object,u16 a_taget_elemrnt);
virtual				~CPHCapture	();

bool				Failed		(){return e_state == cstFree;}

void				Release		();
void				RemoveConnection(CObject* O);

protected:
CPHCharacter		*m_character;
CPhysicsElement*	m_taget_element;
CPhysicsShellHolder*	m_taget_object;
dJointID			m_joint;
dJointID			m_ajoint;
dJointFeedback		m_joint_feedback;
Fvector				m_capture_pos;
float				m_back_force;
float				m_pull_force;
float				m_capture_force;
float				m_capture_distance;
float				m_pull_distance;
u32					m_capture_time;
u32					m_time_start;
CBoneInstance		*m_capture_bone;
dBodyID				m_body;
CPHIsland			m_island;
//bool				b_failed;
bool				b_collide;
bool				b_disabled;
bool				b_character_feedback;

private:
	enum 
	{
	 cstPulling,
	 cstCaptured,
	 cstReleased,
	 cstFree,
	 cstFailed
	} e_state;

			void PullingUpdate();
			void CapturedUpdate();
			void ReleasedUpdate();
			void ReleaseInCallBack();
			void Init( );

			void Deactivate();
			void CreateBody();
			bool Invalid();
static void object_contactCallbackFun(bool& do_colide,bool bo1,dContact& c,SGameMtl* /*material_1*/,SGameMtl* /*material_2*/);

///////////CPHObject/////////////////////////////
	virtual void PhDataUpdate(dReal step);
	virtual void PhTune(dReal step);
	virtual void NetRelcase		(CPhysicsShell *s);

};
#endif