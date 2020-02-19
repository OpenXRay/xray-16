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
ENGINE_API extern xrGUID generate_guid();
ENGINE_API extern const char* generate_guid(const xrGUID& guid, char* buffer, const size_t& buffer_size);
