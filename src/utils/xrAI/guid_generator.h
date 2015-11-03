////////////////////////////////////////////////////////////////////////////
//	Module 		: guid_generator.h
//	Created 	: 21.03.2005
//  Modified 	: 21.03.2005
//	Author		: Dmitriy Iassenev
//	Description : GUID generator
////////////////////////////////////////////////////////////////////////////

#ifndef guid_generatorH
#define guid_generatorH
#pragma once

#include "Common/LevelStructure.hpp"
// XXX: move to xrCore
ENGINE_API extern xrGUID generate_guid();
ENGINE_API extern LPCSTR generate_guid(const xrGUID &guid, LPSTR buffer, const u32 &buffer_size);

#endif //guid_generatorH
