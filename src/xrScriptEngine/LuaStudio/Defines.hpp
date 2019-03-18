////////////////////////////////////////////////////////////////////////////
//	Module 		: defines.h
//	Created 	: 23.04.2008
//  Modified 	: 23.04.2008
//	Author		: Dmitriy Iassenev
//	Description : important macro definitions
////////////////////////////////////////////////////////////////////////////

#pragma once

// DECLSPEC_NOVTABLE macro
#ifndef DECLSPEC_NOVTABLE
#if (_MSC_VER >= 1100) && defined(__cplusplus)
#define DECLSPEC_NOVTABLE __declspec(novtable)
#else // #if (_MSC_VER >= 1100) && defined(__cplusplus)
#define DECLSPEC_NOVTABLE
#endif // #if (_MSC_VER >= 1100) && defined(__cplusplus)
#endif // #ifndef DECLSPEC_NOVTABLE

// CS_STRING_CONCAT macro
#if defined(CS_STRING_CONCAT) || defined(CS_STRING_CONCAT_HELPER)
STATIC_CHECK(false, CS_STRING_CONCAT_or_CS_STRING_CONCAT_HELPER_or_CS_STRING_CONCAT4_macro_already_defined);
#endif // #if defined(CS_STRING_CONCAT) || defined(CS_STRING_CONCAT_HELPER)

#define CS_STRING_CONCAT_HELPER(a, b) a##b
#define CS_STRING_CONCAT(a, b) CS_STRING_CONCAT_HELPER(a, b)

// CS_MAKE_STRING macro
#if defined(CS_MAKE_STRING) || defined(CS_MAKE_STRING_HELPER)
STATIC_CHECK(false, CS_MAKE_STRING_or_CS_MAKE_STRING_HELPER_macro_already_defined);
#endif // #if defined(CS_MAKE_STRING) || defined(CS_MAKE_STRING_HELPER)

#define CS_MAKE_STRING_HELPER(a) #a
#define CS_MAKE_STRING(a) CS_MAKE_STRING_HELPER(a)

#define CS_LIBRARY_NAME(library, extension) \
   CS_MAKE_STRING(CS_LIBRARY_PREFIX) CS_MAKE_STRING(library) CS_MAKE_STRING(CS_PLATFORM_ID) CS_MAKE_STRING(CS_SOLUTION_CONFIGURATION_ID) "." CS_MAKE_STRING(extension)
