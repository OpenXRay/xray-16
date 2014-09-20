////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.h
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "associative_vector.h"

class CInventoryOwner;
class CGameObject;

class CPurchaseList {
public:
	typedef associative_vector<shared_str,float>	DEFICITS;

private:
	DEFICITS				m_deficits;

private:
			void			process			(const CGameObject &owner, const shared_str &name, const u32 &count, const float &probability);

public:
			void			process			(CInifile &ini_file, LPCSTR section, CInventoryOwner &owner);

public:
	IC		void			deficit			(const shared_str &section, const float &deficit);
	IC		float			deficit			(const shared_str &section) const;
	IC		const DEFICITS	&deficits		() const;
};

#include "purchase_list_inline.h"