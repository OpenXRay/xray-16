////////////////////////////////////////////////////////////////////////////
//	Module 		: ef_storage_רעהרעף.h
//	Created 	: 25.03.2002
//  Modified 	: 11.10.2002
//	Author		: Dmitriy Iassenev
//	Description : Evaluation functions storage class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	void CEF_Storage::alife_evaluation		(bool value)
{
	if (value)
		non_alife().clear	();
	else
		alife().clear		();
}

IC	CNonALifeParams &CEF_Storage::non_alife	()
{
	return					(m_non_alife_params);
}

IC	CALifeParams &CEF_Storage::alife		()
{
	return					(m_alife_params);
}
