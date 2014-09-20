#include "stdafx.h"
#include "PHObject.h"
#include "phcollidevalidator.h"

CGID CPHCollideValidator::freeGroupID=0;
_flags<CLClassBits> CPHCollideValidator::ClassFlags={CLClassBits(0)};	
_flags<CLClassBits> CPHCollideValidator::ClassNCFlags={CLClassBits(0)};
_flags<CLClassBits> CPHCollideValidator::NonTypeFlags={CLClassBits(0)};	
void CPHCollideValidator::Init()
{
	freeGroupID=0;
	NonTypeFlags.set(cbNCGroupObject,TRUE);

	ClassFlags.set(cbClassDynamic|cbClassCharacter|cbClassRagDoll|cbClassAnimated,TRUE);
	ClassNCFlags.set(cbNCClassCharacter|cbNCClassDynamic|cbNCClassRagDoll|cbNCClassAnimated,TRUE);

}
CGID CPHCollideValidator::RegisterGroup()
{
	++freeGroupID;
	return freeGroupID-1;
}

void CPHCollideValidator::InitObject(CPHObject& obj)
{
	obj.collide_class_bits().assign(0);
	obj.collide_class_bits().set(cbClassDynamic,TRUE);
	obj.collide_bits()=0;
}
void CPHCollideValidator::RegisterObjToGroup(CGID group,CPHObject& obj)
{
	R_ASSERT(group<freeGroupID);
	obj.collide_bits()=group;
	obj.collide_class_bits().set(cbNCGroupObject,TRUE);
}
bool CPHCollideValidator::IsGroupObject(const CPHObject& obj)
{
	return !!obj.collide_class_bits().test(cbNCGroupObject);
}


bool CPHCollideValidator::IsAnimatedObject(const CPHObject& obj)
{
	return !!obj.collide_class_bits().test(cbClassAnimated);
}


void CPHCollideValidator::RegisterObjToLastGroup(CPHObject& obj)
{
	RegisterObjToGroup(LastGroupRegistred(),obj);
}

CGID CPHCollideValidator::LastGroupRegistred()
{
	return freeGroupID-1;
}

void CPHCollideValidator::RestoreGroupObject(const CPHObject& obj)
{
}

void CPHCollideValidator::SetStaticNotCollide(CPHObject& obj)
{
	obj.collide_class_bits().set(cbNCStatic,TRUE);
}
void CPHCollideValidator::SetDynamicNotCollide(CPHObject& obj)
{
	obj.collide_class_bits().set(cbNCClassDynamic,TRUE);
}

void CPHCollideValidator::SetNonDynamicObject(CPHObject& obj)
{
	obj.collide_class_bits().set(cbClassDynamic,FALSE);
}

void	CPHCollideValidator::SetCharacterClass			(CPHObject& obj)
{
	obj.collide_class_bits().set(cbClassCharacter,TRUE);
}

void	CPHCollideValidator::SetCharacterClassNotCollide	(CPHObject& obj)
{
	obj.collide_class_bits().set(cbNCClassCharacter,TRUE);
}

void	CPHCollideValidator::SetRagDollClass				(CPHObject& obj)
{
	obj.collide_class_bits().set(cbClassRagDoll,TRUE);
}

void	CPHCollideValidator::SetRagDollClassNotCollide		(CPHObject& obj)
{
	obj.collide_class_bits().set(cbNCClassRagDoll,TRUE);
}

	//Относит физический объект к классу анимированных объектов
	void	CPHCollideValidator::SetAnimatedClass				(CPHObject& obj)
	{
		obj.collide_class_bits().set(cbClassAnimated,TRUE);
	}

	//Задаёт игнорирование коллизий данного физического
	//объекта с анимированными телами
	void	CPHCollideValidator::SetAnimatedClassNotCollide		(CPHObject& obj)
	{
		obj.collide_class_bits().set(cbNCClassAnimated,TRUE);
	}

void	CPHCollideValidator::		SetClassSmall				(CPHObject& obj)
{
	obj.collide_class_bits().set(cbClassSmall,TRUE);
}
void	CPHCollideValidator::		SetClassSmallNotCollide		(CPHObject& obj)
{
	obj.collide_class_bits().set(cbNCClassSmall,TRUE);
}