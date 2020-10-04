////////////////////////////////////////////////////////////////////////////
//	Module 		: guid_generator.h
//	Created 	: 21.03.2005
//  Modified 	: 21.03.2005
//	Author		: Dmitriy Iassenev
//	Description : GUID generator
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/GUID.hpp"

// XXX: move to xrCore
xrGUID generate_guid();
LPCSTR generate_guid(const xrGUID& guid, pstr buffer, const size_t& buffer_size);
