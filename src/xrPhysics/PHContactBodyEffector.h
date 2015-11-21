#ifndef PH_CONTACT_BODY_EFFECTOR_H
#define PH_CONTACT_BODY_EFFECTOR_H
#include "PHBaseBodyEffector.h"
#include "ODE/include/ode/contact.h"
struct SGameMtl;
class CPHContactBodyEffector : public CPHBaseBodyEffector
{
dContact m_contact;
float	 m_recip_flotation;
SGameMtl* m_material;
public:
void	Init(dBodyID body,const dContact& contact,SGameMtl* material);
void	Merge(const dContact& contact, SGameMtl* material);
void	Apply();
};
#endif	//PH_CONTACT_BODY_EFFECTOR_H