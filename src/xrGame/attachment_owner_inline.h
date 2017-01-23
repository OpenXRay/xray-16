////////////////////////////////////////////////////////////////////////////
//	Module 		: attachment_owner_inline.h
//	Created 	: 12.02.2004
//  Modified 	: 12.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Attachment owner inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CAttachmentOwner::CAttachmentOwner()
{
}

IC	const xr_vector<CAttachableItem*> &CAttachmentOwner::attached_objects() const
{
	return			(m_attached_objects);
}
