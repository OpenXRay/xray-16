//
//
#ifndef PH_FRACTURE_H
#define PH_FRACTURE_H

#include "PHDefs.h"
#include "PHImpact.h"
#include "ode_include.h"

class CPHFracture;
class CPHElement;

DEFINE_VECTOR(dJointFeedback,CFEEDBACK_STORAGE,CFEEDBACK_I)

IC	void sub_diapasones(u16 &from1,u16 &to1,const u16 &from0,const u16 &to0);

class CShellSplitInfo 
{
	friend class  CPHFracturesHolder;
	friend class  CPHShellSplitterHolder;
	friend class CPHElement;
	IC bool HaveElements		()		{return m_end_el_num!=m_start_el_num;}
	IC bool HaveJoints			()		{return m_start_jt_num!=m_end_jt_num;}
public:
	IC void sub_diapasone(const CShellSplitInfo& sub)
	{
		sub_diapasones(m_start_el_num,m_end_el_num,sub.m_start_el_num,sub.m_end_el_num);
		sub_diapasones(m_start_jt_num,m_end_jt_num,sub.m_start_jt_num,sub.m_end_jt_num);
	}
protected:
	u16				m_start_el_num;
	u16				m_end_el_num;
	u16				m_start_jt_num;
	u16				m_end_jt_num;
	u16				m_start_geom_num;
	u16				m_end_geom_num;
	u16				m_bone_id;
};

class CPHFracture : public CShellSplitInfo
{
	friend class  CPHFracturesHolder;
	friend class CPHElement;
	friend class CPHShell;
	bool			m_breaked;
	dMass			m_firstM;
	dMass			m_secondM;
	//when breaked m_pos_in_element-additional force m_break_force-additional torque -x additional torque-y add_torque_z - additional torque z
	float			m_break_force;
	float			m_break_torque;
	Fvector			m_pos_in_element;
	float			m_add_torque_z;
	CPHFracture();
public:
	bool			Update(CPHElement* element);
	IC bool			Breaked(){return m_breaked;}
	void			SetMassParts(const dMass& first,const dMass& second);
	void			MassSetZerro();
	void			MassAddToFirst(const dMass& m);
	void			MassAddToSecond(const dMass& m);
	void			MassSubFromFirst(const dMass& m);
	void			MassSubFromSecond(const dMass& m);
	void			MassSetFirst(const dMass& m);
	void			MassSetSecond(const dMass& m);
	const dMass&	MassFirst(){return m_firstM;}
	const dMass&	MassSecond(){return m_secondM;}
	void			MassUnsplitFromFirstToSecond(const dMass& m);
};

DEFINE_VECTOR(CPHFracture,FRACTURE_STORAGE,FRACTURE_I)
typedef std::pair<CPHElement*,CShellSplitInfo>	element_fracture;
typedef		xr_vector<element_fracture>::reverse_iterator	ELEMENT_PAIR_RI;
typedef		xr_vector<CPHFracture>::reverse_iterator	FRACTURE_RI;
DEFINE_VECTOR(element_fracture,ELEMENT_PAIR_VECTOR,ELEMENT_PAIR_I)

class CPHFracturesHolder 			//stored in CPHElement
{
friend class CPHElement;
friend class CPHShellSplitterHolder;
bool			 m_has_breaks;

FRACTURE_STORAGE m_fractures;
PH_IMPACT_STORAGE m_impacts;		//filled in anytime from CPHElement applyImpulseTrace cleared in PhDataUpdate
CFEEDBACK_STORAGE m_feedbacks;		//this store feedbacks for non contact joints 
public:
CPHFracturesHolder			();

~CPHFracturesHolder			();
void				DistributeAdditionalMass	(u16 geom_num,const dMass& m);//
void				SubFractureMass				(u16 fracture_num);
void				AddImpact		(const Fvector& force,const Fvector& point,u16 id);
PH_IMPACT_STORAGE&	Impacts			(){return m_impacts;}

CPHFracture&		LastFracture	(){return m_fractures.back();}
protected:
private:

u16 				CheckFractured	();										//returns first breaked fracture

element_fracture	SplitFromEnd	(CPHElement* element,u16 geom_num);
void				InitNewElement	(CPHElement* element,const Fmatrix &shift_pivot,float density);
void				PassEndFractures(u16 from,CPHElement* dest);
public:
void				SplitProcess	(CPHElement* element,ELEMENT_PAIR_VECTOR &new_elements);
u16					AddFracture		(const CPHFracture& fracture);
CPHFracture&		Fracture		(u16 num);
void				PhTune			(dBodyID body);										//set feedback for joints called from PhTune of ShellSplitterHolder
bool				PhDataUpdate	(CPHElement* element);										//collect joints and external impacts in fractures Update which set m_fractured; called from PhDataUpdate of ShellSplitterHolder returns true if has breaks
void				ApplyImpactsToElement(CPHElement* element);
};

IC	void sub_diapasones(u16 &from1,u16 &to1,const u16 &from0,const u16 &to0)
{
	if(from0==to0 ||from1==to1|| to1<=from0||to1==u16(-1)) return;
	R_ASSERT(from0>=from1&&to0<=to1);
	u16 dip=to0-from0;
	to1=to1-dip;
}

#endif  PH_FRACTURE_H